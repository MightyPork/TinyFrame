//---------------------------------------------------------------------------
#include "TinyFrame.h"
#include <string.h>
//---------------------------------------------------------------------------

enum TFState {
	TFState_SOF = 0,      //!< Wait for SOF
	TFState_LEN,          //!< Wait for Number Of Bytes
	TFState_HEAD_CKSUM,   //!< Wait for header Checksum
	TFState_ID,           //!< Wait for ID
	TFState_TYPE,         //!< Wait for message type
	TFState_DATA,         //!< Receive payload
	TFState_DATA_CKSUM,   //!< Wait for Checksum
};

typedef struct _IdListener_struct {
	unsigned int id;
	TF_LISTENER fn;
} IdListener;

typedef struct _TypeListener_struct_ {
	unsigned char type;
	TF_LISTENER fn;
} TypeListener;

typedef struct _GenericListener_struct_ {
	TF_LISTENER fn;
} GenericListener;

/**
 * Frame parser internal state
 */
static struct TinyFrameStruct {
	/* Own state */
	TF_PEER peer_bit;       //!< Own peer bit (unqiue to avoid msg ID clash)
	TF_ID next_id;          //!< Next frame / frame chain ID

	/* Parser state */
	enum TFState state;
	int parser_timeout_ticks;
	TF_ID id;               //!< Incoming packet ID
	TF_LEN len;             //!< Payload length
	uint8_t data[TF_MAX_PAYLOAD]; //!< Data byte buffer
	size_t rxi;             //!< Byte counter
	TF_CKSUM cksum;         //!< Checksum calculated of the data stream
	TF_CKSUM ref_cksum;     //!< Reference checksum read from the message
	TF_TYPE type;           //!< Collected message type number

	/* --- Callbacks --- */

	/* Transaction callbacks */
	IdListener id_listeners[TF_MAX_ID_LST];
	TypeListener type_listeners[TF_MAX_TYPE_LST];
	GenericListener generic_listeners[TF_MAX_GEN_LST];

	size_t count_id_lst;
	size_t count_type_lst;
	size_t count_generic_lst;

	uint8_t sendbuf[TF_MAX_PAYLOAD + TF_OVERHEAD_BYTES];
} tf;


//region Optional impls
#if TF_USE_CRC16

// ---- CRC16 checksum impl ----

/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
const uint16_t crc16_table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static inline uint16_t crc16_byte(uint16_t crc, const uint8_t data)
{
	return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

#endif
//endregion Optional impls

// --- macros based on config ---

#define CKSUM_RESET(cksum) do { cksum = 0; } while (0)

#if TF_USE_CRC16
	// CRC16 checksum
	#define CKSUM_ADD(cksum, byte) do { cksum = crc16_byte(cksum, byte); } while(0)
#else
	// XOR checksum
	#define CKSUM_ADD(cksum, byte) do { cksum ^= byte; } while(0)
#endif


void TF_Init(TF_PEER peer_bit)
{
	// Zero it out
	memset(&tf, 0, sizeof(struct TinyFrameStruct));

	tf.peer_bit = peer_bit;
}

//region Listeners

bool TF_AddIdListener(TF_ID frame_id, TF_LISTENER cb)
{
	size_t i;
	for (i = 0; i < TF_MAX_ID_LST; i++) {
		if (tf.id_listeners[i].fn == NULL) {
			tf.id_listeners[i].fn = cb;
			tf.id_listeners[i].id = frame_id;
			if (i >= tf.count_id_lst) {
				tf.count_id_lst = i + 1;
			}
			return true;
		}
	}
	return false;
}

bool TF_AddTypeListener(unsigned char frame_type, TF_LISTENER cb)
{
	size_t i;
	for (i = 0; i < TF_MAX_TYPE_LST; i++) {
		if (tf.type_listeners[i].fn == NULL) {
			tf.type_listeners[i].fn = cb;
			tf.type_listeners[i].type = frame_type;
			if (i >= tf.count_type_lst) {
				tf.count_type_lst = i + 1;
			}
			return true;
		}
	}
	return false;
}

bool TF_AddGenericListener(TF_LISTENER cb)
{
	size_t i;
	for (i = 0; i < TF_MAX_GEN_LST; i++) {
		if (tf.generic_listeners[i].fn == NULL) {
			tf.generic_listeners[i].fn = cb;
			if (i >= tf.count_generic_lst) {
				tf.count_generic_lst = i + 1;
			}
			return true;
		}
	}
	return false;
}

bool TF_RemoveIdListener(TF_ID frame_id)
{
	size_t i;
	for (i = 0; i < tf.count_id_lst; i++) {
		if (tf.id_listeners[i].fn != NULL
			&& tf.id_listeners[i].id == frame_id) {
			tf.id_listeners[i].fn = NULL;
			if (i == tf.count_id_lst - 1) {
				tf.count_id_lst--;
			}
			return true;
		}
	}
	return false;
}

bool TF_RemoveTypeListener(unsigned char type)
{
	size_t i;
	for (i = 0; i < tf.count_type_lst; i++) {
		if (tf.type_listeners[i].fn != NULL
			&& tf.type_listeners[i].type == type) {
			tf.type_listeners[i].fn = NULL;
			if (i == tf.count_type_lst - 1) {
				tf.count_type_lst--;
			}
			return true;
		}
	}
	return false;
}

bool TF_RemoveGenericListener(TF_LISTENER cb)
{
	size_t i;
	for (i = 0; i < tf.count_generic_lst; i++) {
		if (tf.generic_listeners[i].fn == cb) {
			tf.generic_listeners[i].fn = NULL;
			if (i == tf.count_generic_lst - 1) {
				tf.count_generic_lst--;
			}
			return true;
		}
	}
	return false;
}

/** Handle a message that was just collected & verified by the parser */
static void TF_HandleReceivedMessage(TF_ID frame_id, TF_TYPE type, uint8_t *data)
{
	size_t i;

	// Any listener can consume the message (return true),
	// or let someone else handle it.

	// The loop upper bounds are the highest currently used slot index
	// (or close to it, depending on the order of listener removals)

	// ID listeners first
	for (i = 0; i < tf.count_id_lst; i++) {
		if (tf.id_listeners[i].fn && tf.id_listeners[i].id == tf.id) {
			if (tf.id_listeners[i].fn(tf.id, tf.type, tf.data, tf.len)) {
				return;
			}
		}
	}

	// Type listeners
	for (i = 0; i < tf.count_type_lst; i++) {
		if (tf.type_listeners[i].fn && tf.type_listeners[i].type == tf.type) {
			if (tf.type_listeners[i].fn(tf.id, tf.type, tf.data, tf.len)) {
				return;
			}
		}
	}

	// Generic listeners
	for (i = 0; i < tf.count_generic_lst; i++) {
		if (tf.generic_listeners[i].fn) {
			if (tf.generic_listeners[i].fn(tf.id, tf.type, tf.data, tf.len)) {
				return;
			}
		}
	}
}

//endregion Listeners

void TF_Accept(const uint8_t *buffer, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++) {
		TF_AcceptChar(buffer[i]);
	}
}

void TF_ResetParser(void)
{
	tf.state = TFState_SOF;
}

void TF_AcceptChar(unsigned char c)
{
	// QUEUE IF PARSER LOCKED
	// FIRST PROCESS ALL QUEUED

	// Timeout - clear
	if (tf.parser_timeout_ticks >= TF_PARSER_TIMEOUT_TICKS) {
		TF_ResetParser();
	}
	tf.parser_timeout_ticks = 0;

// DRY snippet - collect multi-byte number from the input stream
#define COLLECT_NUMBER(dest, type) dest = ((dest) << 8) | c; \
								   if (++tf.rxi == sizeof(type))

	switch (tf.state) {
		case TFState_SOF:
			if (c == TF_SOF_BYTE) {
				// Enter LEN state
				tf.state = TFState_LEN;
				tf.len = 0;
				tf.rxi = 0;

				// Reset state vars
				CKSUM_RESET(tf.cksum);
			}
			break;

		case TFState_LEN:
			CKSUM_ADD(tf.cksum, c);
			COLLECT_NUMBER(tf.len, TF_LEN) {
				// enter HEAD_CKSUM state
				tf.state = TFState_HEAD_CKSUM;
				tf.ref_cksum = 0;
				tf.rxi = 0;
			}
			break;

		case TFState_HEAD_CKSUM:
			COLLECT_NUMBER(tf.ref_cksum, TF_CKSUM){
				// Check the header checksum against the computed value
				if (tf.cksum != tf.ref_cksum) {
					TF_ResetParser();
					break;
				}

				// Enter ID state
				tf.state = TFState_ID;
				tf.rxi = 0;

				tf.cksum = 0; // Start collecting the payload
			}
			break;

		case TFState_ID:
			CKSUM_ADD(tf.cksum, c);
			COLLECT_NUMBER(tf.id, TF_ID) {
				// Enter TYPE state
				tf.state = TFState_TYPE;
				tf.rxi = 0;
			}
			break;

		case TFState_TYPE:
			CKSUM_ADD(tf.cksum, c);
			COLLECT_NUMBER(tf.type, TF_TYPE) {
				// Enter DATA state
				tf.state = TFState_DATA;
				tf.rxi = 0;
			}
			break;

		case TFState_DATA:
			CKSUM_ADD(tf.cksum, c);
			tf.data[tf.rxi++] = c;
			if (tf.rxi == tf.len) {
				// Enter DATA_CKSUM state
				tf.state = TFState_DATA_CKSUM;
				tf.rxi = 0;
				tf.ref_cksum = 0;
			}
			break;

		case TFState_DATA_CKSUM:
			COLLECT_NUMBER(tf.ref_cksum, TF_CKSUM) {
				// Check the header checksum against the computed value
				if (tf.cksum == tf.ref_cksum) {
					// LOCK PARSER
					TF_HandleReceivedMessage(tf.id, tf.type, tf.data);
					// UNLOCK PARSER
				}

				TF_ResetParser();
			}
			break;
	}
}

int TF_Compose(uint8_t *outbuff, TF_ID *id_ptr,
			      TF_TYPE type,
				  const uint8_t *data, TF_LEN data_len,
				  TF_ID explicit_id, bool use_expl_id)
{
	int i;
	uint8_t b;
	TF_ID id;
	TF_CKSUM cksum;
	int pos = 0;

	CKSUM_RESET(cksum);

	// sanitize len
	if (data_len > TF_MAX_PAYLOAD) {
		return TF_ERROR;
	}

	// Gen ID
	if (use_expl_id) {
		id = explicit_id;
	}
	else {
		id = (TF_ID) ((tf.next_id++) & TF_ID_MASK);
		if (tf.peer_bit) {
			id |= TF_ID_PEERBIT;
		}
	}

	// DRY helper for writing a multi-byte variable to the buffer
#define WRITENUM_BASE(type, num, xtra) \
	for (i = sizeof(type)-1; i>=0; i--) { \
		b = (uint8_t)(num >> (i*8) & 0xFF); \
		outbuff[pos++] = b; \
		xtra; \
	}

#define WRITENUM(type, num)       WRITENUM_BASE(type, num, )
#define WRITENUM_CKSUM(type, num) WRITENUM_BASE(type, num, CKSUM_ADD(cksum, b))

	// --- Start ---

	outbuff[pos++] = TF_SOF_BYTE;
	cksum = 0;
	WRITENUM_CKSUM(TF_LEN, data_len);
	WRITENUM(TF_CKSUM, cksum);

	// --- payload begin ---
	cksum = 0;
	WRITENUM_CKSUM(TF_ID, id);
	WRITENUM_CKSUM(TF_TYPE, type);

	// DATA
	for (i = 0; i < data_len; i++) {
		b = data[i];
		outbuff[pos++] = b;
		CKSUM_ADD(cksum, b);
	}

	WRITENUM(TF_CKSUM, cksum);

	if (id_ptr != NULL)
		*id_ptr = id;

	return pos;
}

bool TF_Send(TF_TYPE type, const uint8_t *payload, TF_LEN payload_len,
			TF_LISTENER listener, TF_ID *id_ptr)
{
	TF_ID msgid;
	int len;
	len = TF_Compose(tf.sendbuf, &msgid, type, payload, payload_len, 0, false);
	if (len == TF_ERROR) return false;

	if (listener) TF_AddIdListener(msgid, listener);
	if (id_ptr) *id_ptr = msgid;

	TF_WriteImpl((const uint8_t *) tf.sendbuf, (TF_LEN)len);
	return true;
}

bool TF_Respond(TF_TYPE type,
				const uint8_t *data, TF_LEN data_len,
				TF_ID frame_id)
{
	int len;
	len = TF_Compose(tf.sendbuf, NULL, type, data, data_len, frame_id, true);
	if (len == TF_ERROR) return false;

	TF_WriteImpl(tf.sendbuf, (TF_LEN)len);
	return true;
}

/**
 * Like TF_Send(), but with just 1 data byte
 */
bool TF_Send1(TF_TYPE type, uint8_t b1,
			  TF_LISTENER listener,
			  TF_ID *id_ptr)
{
	unsigned char b[] = {b1};
	return TF_Send(type, b, 1, listener, id_ptr);
}

/**
 * Like TF_Send(), but with just 2 data bytes
 */
bool TF_Send2(TF_TYPE type, uint8_t b1, uint8_t b2,
			  TF_LISTENER listener,
			  TF_ID *id_ptr)
{
	unsigned char b[] = {b1, b2};
	return TF_Send(type, b, 2, listener, id_ptr);
}

void TF_Tick(void)
{
	if (tf.parser_timeout_ticks < TF_PARSER_TIMEOUT_TICKS) {
		tf.parser_timeout_ticks++;
	}
}

