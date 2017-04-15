#ifndef TinyFrameH
#define TinyFrameH
//---------------------------------------------------------------------------
#include <stdint.h>  // for uint8_t etc
#include <stdbool.h> // for bool
#include <stdlib.h>  // for NULL
//---------------------------------------------------------------------------

// A static buffer of this size will be allocated
#define TF_MAX_PAYLOAD 2048

// Frame ID listeners (wait for response / multi-part message)
#define TF_MAX_ID_LST   20
// Frame Type listeners (wait for frame with a specific first payload byte)
#define TF_MAX_TYPE_LST 20
// Generic listeners (fallback if no other listener catches it)
#define TF_MAX_GEN_LST  4

// Timeout for receiving & parsing a frame
// ticks = number of calls to TF_Tick()
#define TF_PARSER_TIMEOUT_TICKS 10

#define TF_USE_CRC16 1

//---------------------------------------------------------------------------

// ,-----+-----+-----------+----+------+- - - -+------------,
// | SOF | LEN | LEN_CKSUM | ID | TYPE | DATA  | PLD_CKSUM  |
// | 1   | 2?  | 1?        | 1? | 1?   | ...   | 1?         |
// '-----+-----+-----------+----+------+- - - -+------------'
//                         '----- payload -----'

// The data section is designed thus so the higher levels of TinyFrame
// (message chaining, listeners etc) could be re-used for a different
// framing layer (e.g. sent in UDP packets)

// Fields marked '?' can have their size adjusted by changing the
// typedefs below

// Change those typedefs to adjust the field sizes. BOTH PEERS MUST HAVE THE SAME SIZES!
typedef uint8_t TF_ID;   // Message ID. Effectively limits the nr of concurrent request-response sessions.
typedef uint8_t TF_TYPE; // Message type (sent together with the data payload, which can be empty)
typedef uint16_t TF_LEN; // Length of the data payload.

// TF_OVERHEAD_BYTES - added to TF_MAX_PAYLOAD for the send buffer size.
// (TODO - buffer-less sending)
#if TF_USE_CRC16
	typedef uint16_t TF_CKSUM;
	#define TF_OVERHEAD_BYTES 9
#else
	typedef uint8_t TF_CKSUM;
	#define TF_OVERHEAD_BYTES 7
#endif

//---------------------------------------------------------------------------

#define TF_SOF_BYTE 0x01

#define TF_ERROR -1

// Type-dependent masks for bit manipulation in the ID field
#define TF_ID_MASK ((1 << (sizeof(TF_ID)*8 - 1)) - 1)
#define TF_ID_PEERBIT (1 << (sizeof(TF_ID)*8) - 1)

typedef enum {
	TF_SLAVE = 0,
	TF_MASTER = 1,
} TF_PEER;

//---------------------------------------------------------------------------

/**
 * TinyFrame Type Listener callback
 * @param frame_id - ID of the received frame
 * @param data - byte buffer with the application data
 * @param len - number of bytes in the buffer
 * @return true if the frame was consumed
 */
typedef bool (*TF_LISTENER)(TF_ID frame_id,
							TF_TYPE type,
							const uint8_t *data, TF_LEN len);

/**
 * Initialize the TinyFrame engine.
 * This can also be used to completely reset it (removing all listeners etc)
 * @param peer_bit - peer bit to use for self
 */
void TF_Init(TF_PEER peer_bit);

/**
 * Reset the frame parser state machine.
 */
void TF_ResetParser(void);

/**
 * Accept incoming bytes & parse frames
 * @param buffer - byte buffer to process
 * @param count - nr of bytes in the buffer
 */
void TF_Accept(const uint8_t *buffer, size_t count);

/**
 * Accept a single incoming byte
 * @param c - a received char
 */
void TF_AcceptChar(uint8_t c);

/**
 * Register a frame type listener.
 * @param frame_type - frame ID to listen for
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddIdListener(TF_ID frame_id, TF_LISTENER cb);

/**
 * Remove a listener by the message ID it's registered for
 * @param frame_id - the frame we're listening for
 */
bool TF_RemoveIdListener(TF_ID frame_id);

/**
 * Register a frame type listener.
 * @param frame_type - frame type to listen for
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddTypeListener(TF_TYPE frame_type, TF_LISTENER cb);

/**
 * Remove a listener by type.
 * @param type - the type it's registered for
 */
bool TF_RemoveTypeListener(TF_TYPE type);

/**
 * Register a generic listener.
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddGenericListener(TF_LISTENER cb);

/**
 * Remove a generic listener by function pointer
 * @param cb - callback function to remove
 */
bool TF_RemoveGenericListener(TF_LISTENER cb);

/**
 * Compose a frame (used internally by TF_Send and TF_Respond).
 * The frame can be sent using TF_WriteImpl(), or received by TF_Accept()
 *
 * @param outbuff - buffer to store the result in
 * @param msgid - message ID is stored here, if not NULL
 * @param type - message type
 * @param data - data buffer
 * @param len - payload size in bytes
 * @param explicit_id - ID to use in the frame (8-bit)
 * @param use_expl_id - whether to use the previous param
 * @return nr of bytes in outbuff used by the frame, TF_ERROR (-1) on failure
 */
int TF_Compose(uint8_t *outbuff,
			   TF_ID *id_ptr,
			   TF_TYPE type,
			   const uint8_t *data, TF_LEN data_len,
			   TF_ID explicit_id, bool use_expl_id);

/**
 * Send a frame, and optionally attach an ID listener.
 *
 * @param type - message type
 * @param data - data to send (can be NULL if 'data_len' is 0)
 * @param data_len - nr of bytes to send
 * @param listener - listener waiting for the response
 * @param id_ptr - store the ID here, NULL to don't store.
 *                 The ID may be used to unbind the listener after a timeout.
 * @return success
 */
bool TF_Send(TF_TYPE type, const uint8_t *data, TF_LEN data_len,
			 TF_LISTENER listener,
			 TF_ID *id_ptr);

/**
 * Like TF_Send(), but with just 1 data byte
 */
bool TF_Send1(TF_TYPE type, uint8_t b1,
			  TF_LISTENER listener,
			  TF_ID *id_ptr);
/**
 * Like TF_Send(), but with just 2 data bytes
 */
bool TF_Send2(TF_TYPE type, uint8_t b1, uint8_t b2,
			  TF_LISTENER listener,
			  TF_ID *id_ptr);

/**
 * Send a response to a received message.
 *
 * @param type - message type. If an ID listener is waiting for this response,
 *               then 'type' can be used to pass additional information.
 *               Otherwise, 'type' can be used to handle the message using a TypeListener.
 * @param data - data to send
 * @param data_len - nr of bytes to send
 * @param frame_id - ID of the response frame (re-use ID from the original message)
 * @return success
 */
bool TF_Respond(TF_TYPE type,
				const uint8_t *data, TF_LEN data_len,
				TF_ID frame_id);

/**
 * 'Write bytes' function that sends data to UART
 *
 * ! Implement this in your application code !
 */
extern void TF_WriteImpl(const uint8_t *buff, TF_LEN len);

/**
 * This function must be called periodically.
 *
 * The time base is used to time-out partial frames in the parser and
 * automatically reset it.
 *
 * (suggestion - call this in a SysTick handler)
 */
void TF_Tick(void);

#endif
