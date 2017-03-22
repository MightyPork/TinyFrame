//---------------------------------------------------------------------------
#include "TinyFrame.h"
#include <string.h>
//---------------------------------------------------------------------------

/* Note: payload length determines the Rx buffer size. Max 256 */
#define TF_MAX_PAYLOAD 256
#define TF_MAX_CALLBACKS 16
#define TF_SOF_BYTE 0x01


enum TFState {
	TFState_SOF = 0,  //!< Wait for SOF
	TFState_ID,       //!< Wait for ID
	TFState_NOB,      //!< Wait for Number Of Bytes
	TFState_PAYLOAD,  //!< Receive payload
	TFState_CKSUM     //!< Wait for Checksum
};


/**
 * Frame parser internal state
 */
static struct TinyFrameStruct {
	/* Own state */
	bool peer_bit;          //!< Own peer bit (unqiue to avoid msg ID clash)
	unsigned int next_id;   //!< Next frame / frame chain ID

	/* Parser state */
	enum TFState state;
	unsigned int id;        //!< Incoming packet ID
	unsigned int nob;       //!< Payload length
	unsigned char pldbuf[TF_MAX_PAYLOAD+1]; //!< Payload byte buffer
	unsigned int rxi;       //!< Receive counter (for payload or other fields)
	unsigned int cksum;     //!< Continually updated checksum

	/* Callbacks */
	int cb_ids[TF_MAX_CALLBACKS]; //!< Callback frame IDs, -1 = all
	TinyFrameListener cb_funcs[TF_MAX_CALLBACKS]; //!< Callback funcs, 0 = free slot
} tf;


/**
 * Initialize the TinyFrame engine.
 * This can also be used to completely reset it (removing all listeners etc)
 *
 * @param peer_bit - peer bit to use for self
 */
void TF_Init(bool peer_bit)
{
	memset(&tf, 0, sizeof(struct TinyFrameStruct));

	tf.peer_bit = peer_bit;
}


/**
 * Reset the frame parser state machine.
 */
void TF_ResetParser(void)
{
	tf.state = TFState_SOF;
	tf.cksum = 0;
}


/**
 * Register a receive callback.
 *
 * @param frame_id - ID of the frame to receive
 * @param cb - callback func
 * @return Callback slot index (for removing). TF_ERROR (-1) on failure
 */
int TF_AddListener(int frame_id, TinyFrameListener cb)
{
	int i;

	for (i = 0; i < TF_MAX_CALLBACKS; i++) {
		if (tf.cb_funcs[i] == NULL) {
			tf.cb_funcs[i] = cb;
			tf.cb_ids[i] = frame_id;
			return i;
		}
	}

	return TF_ERROR;
}


/**
 * Remove a rx callback function by the index received when registering it
 *
 * @param index - index in the callbacks table
 */
void TF_RemoveListener(int index)
{
	tf.cb_funcs[index] = NULL;
}


/**
 * Remove a callback by the function pointer
 *
 * @param cb - callback function to remove
 */
void TF_RemoveListenerFn(TinyFrameListener cb)
{
	int i;

	for (i = 0; i < TF_MAX_CALLBACKS; i++) {
		if (tf.cb_funcs[i] == cb) {
			tf.cb_funcs[i] = NULL;
			// no break, it can be here multiple times
		}
	}

	return;
}


/**
 * Accept incoming bytes & parse frames
 *
 * @param buffer - byte buffer to process
 * @param count - nr of bytes in the buffer
 */
void TF_Accept(const unsigned char *buffer, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		TF_AcceptChar(buffer[i]);
	}
}


/**
 * Process a single received character
 *
 * @param c - rx character
 */
void TF_AcceptChar(unsigned char c)
{
	int i;
	bool rv;

	switch (tf.state)
	{
	case TFState_SOF:
		if (c == TF_SOF_BYTE) {
			tf.cksum = 0;
			tf.state = TFState_ID;
		}
		break;

	case TFState_ID:
		tf.id = c;
		tf.state = TFState_NOB;
		break;

	case TFState_NOB:
		tf.nob = c + 1; // using 0..255 as 1..256
		tf.state = TFState_PAYLOAD;
		tf.rxi = 0;
		break;

	case TFState_PAYLOAD:
		tf.pldbuf[tf.rxi++] = c;
		if (tf.rxi == tf.nob) {
			tf.state = TFState_CKSUM;
		}
		break;

	case TFState_CKSUM:
		tf.state = TFState_SOF;

		if (tf.cksum == (unsigned int)c) {
			// Add 0 at the end of the data in the buffer (useful if it was a string)
			tf.pldbuf[tf.rxi] = '\0';

			// Fire listeners
			for (i = 0; i < TF_MAX_CALLBACKS; i++) {
				// Fire if used & matches
				if (tf.cb_funcs[i] && (tf.cb_ids[i] == -1 || tf.cb_ids[i] == tf.id)) {
					rv = tf.cb_funcs[i](tf.id, tf.pldbuf, tf.nob);

					// Unbind
					if (rv) tf.cb_funcs[i] = NULL;
				}
			}
		} else {
			// Fail, return to base state
			tf.state = TFState_SOF;
        }
		break;
	}

	// Update the checksum
	tf.cksum ^= c;
}


/**
 * Compose a frame
 *
 * @param outbuff - buffer to store the result in
 * @param msgid - message ID is stored here, if not NULL
 * @param payload - data buffer
 * @param len - payload size in bytes, 0 to use strlen
 * @param explicit_id - ID to use, -1 to chose next free
 * @return nr of bytes in outbuff used by the frame, -1 on failure
 */
int TF_Compose(unsigned char *outbuff, unsigned int *msgid,
               const unsigned char *payload, unsigned int payload_len,
               int explicit_id
) {
	unsigned int i;
	unsigned int id;
	int xor;

	// sanitize len
	if (payload_len > TF_MAX_PAYLOAD) return TF_ERROR;
	if (payload_len == 0) payload_len = strlen(payload);

    // Gen ID
	if (explicit_id == TF_NEXT_ID) {
		id = tf.next_id++;
		if (tf.peer_bit) {
			id |= 0x80;
		}

		if (tf.next_id > 0x7F) {
			tf.next_id = 0;
		}
	} else {
		id = explicit_id;
	}

	outbuff[0] = TF_SOF_BYTE;
	outbuff[1] = id & 0xFF;
	outbuff[2] = (payload_len - 1) & 0xFF; // use 0..255 as 1..256
	memcpy(outbuff+3, payload, payload_len);

	xor = 0;
	for (i = 0; i < payload_len + 3; i++) {
		xor ^= outbuff[i];
	}

	outbuff[payload_len + 3] = xor;

	if (msgid != NULL) *msgid = id;

	return payload_len + 4;
}
