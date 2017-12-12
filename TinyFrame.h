#ifndef TinyFrameH
#define TinyFrameH

/**
 * TinyFrame protocol library
 * 
 * (c) Ondřej Hruška 2017, MIT License 
 * no liability/warranty, free for any use, must retain this notice & license
 * 
 * Upstream URL: https://github.com/MightyPork/TinyFrame
 */

#define TF_VERSION "1.2.0"

//---------------------------------------------------------------------------
#include <stdint.h>  // for uint8_t etc
#include <stdbool.h> // for bool
#include <stdlib.h>  // for NULL
//---------------------------------------------------------------------------

// Select checksum type (0 = none, 8 = ~XOR, 16 = CRC16 0x8005, 32 = CRC32)
#define TF_CKSUM_NONE  0
#define TF_CKSUM_XOR   8
#define TF_CKSUM_CRC16 16
#define TF_CKSUM_CRC32 32

#include <TF_Config.h>

//region Resolve data types

#if TF_LEN_BYTES == 1
	typedef uint8_t TF_LEN;
#elif TF_LEN_BYTES == 2
	typedef uint16_t TF_LEN;
#elif TF_LEN_BYTES == 4
	typedef uint32_t TF_LEN;
#else
	#error Bad value of TF_LEN_BYTES, must be 1, 2 or 4
#endif


#if TF_TYPE_BYTES == 1
	typedef uint8_t TF_TYPE;
#elif TF_TYPE_BYTES == 2
	typedef uint16_t TF_TYPE;
#elif TF_TYPE_BYTES == 4
	typedef uint32_t TF_TYPE;
#else
	#error Bad value of TF_TYPE_BYTES, must be 1, 2 or 4
#endif


#if TF_ID_BYTES == 1
	typedef uint8_t TF_ID;
#elif TF_ID_BYTES == 2
	typedef uint16_t TF_ID;
#elif TF_ID_BYTES == 4
	typedef uint32_t TF_ID;
#else
	#error Bad value of TF_ID_BYTES, must be 1, 2 or 4
#endif


#if TF_CKSUM_TYPE == TF_CKSUM_XOR || TF_CKSUM_TYPE == TF_CKSUM_NONE
	// ~XOR (if 0, still use 1 byte - it won't be used)
	typedef uint8_t TF_CKSUM;
#elif TF_CKSUM_TYPE == TF_CKSUM_CRC16
	// CRC16
	typedef uint16_t TF_CKSUM;
#elif TF_CKSUM_TYPE == TF_CKSUM_CRC32
	// CRC32
	typedef uint32_t TF_CKSUM;
#else
	#error Bad value for TF_CKSUM_TYPE, must be 8, 16 or 32
#endif

//endregion

//---------------------------------------------------------------------------

// Type-dependent masks for bit manipulation in the ID field
#define TF_ID_MASK (TF_ID)(((TF_ID)1 << (sizeof(TF_ID)*8 - 1)) - 1)
#define TF_ID_PEERBIT (TF_ID)((TF_ID)1 << ((sizeof(TF_ID)*8) - 1))

//---------------------------------------------------------------------------

/** Peer bit enum (used for init) */
typedef enum {
	TF_SLAVE = 0,
	TF_MASTER,
} TF_Peer;

typedef enum {
	TF_NEXT = 0,   //!< Not handled, let other listeners handle it
	TF_STAY = 1,   //!< Handled, stay
	TF_RENEW = 2,  //!< Handled, stay, renew - useful only with listener timeout
	TF_CLOSE = 3,  //!< Handled, remove self
} TF_Result;

/** Data structure for sending / receiving messages */
typedef struct _TF_MSG_STRUCT_ {
	TF_ID frame_id;       //!< message ID
	bool is_response;     //!< internal flag, set when using the Respond function. frame_id is then kept unchanged.
	TF_TYPE type;         //!< received or sent message type
	const uint8_t *data;  //!< buffer of received data or data to send. NULL = listener timed out, free userdata!
	TF_LEN len;           //!< length of the buffer
	void *userdata;       //!< here's a place for custom data; this data will be stored with the listener
	void *userdata2;
} TF_Msg;

/**
 * Clear message struct
 */
static inline void TF_ClearMsg(TF_Msg *msg)
{
	msg->frame_id = 0;
	msg->is_response = false;
	msg->type = 0;
	msg->data = NULL;
	msg->len = 0;
	msg->userdata = NULL;
	msg->userdata2 = NULL;
}

/**
 * TinyFrame Type Listener callback
 *
 * @param frame_id - ID of the received frame
 * @param type - type field from the message
 * @param data - byte buffer with the application data
 * @param len - number of bytes in the buffer
 * @return listener result
 */
typedef TF_Result (*TF_Listener)(TF_Msg *msg);

/**
 * Initialize the TinyFrame engine.
 * This can also be used to completely reset it (removing all listeners etc)
 *
 * @param peer_bit - peer bit to use for self
 */
void TF_Init(TF_Peer peer_bit);

/**
 * Reset the frame parser state machine.
 * This does not affect registered listeners.
 */
void TF_ResetParser(void);

/**
 * Register a frame type listener.
 *
 * @param msg - message (contains frame_id and userdata)
 * @param cb - callback
 * @param timeout - timeout in ticks to auto-remove the listener (0 = keep forever)
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddIdListener(TF_Msg *msg, TF_Listener cb, TF_TICKS timeout);

/**
 * Remove a listener by the message ID it's registered for
 *
 * @param frame_id - the frame we're listening for
 */
bool TF_RemoveIdListener(TF_ID frame_id);

/**
 * Register a frame type listener.
 *
 * @param frame_type - frame type to listen for
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddTypeListener(TF_TYPE frame_type, TF_Listener cb);

/**
 * Remove a listener by type.
 *
 * @param type - the type it's registered for
 */
bool TF_RemoveTypeListener(TF_TYPE type);

/**
 * Register a generic listener.
 *
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddGenericListener(TF_Listener cb);

/**
 * Remove a generic listener by function pointer
 *
 * @param cb - callback function to remove
 */
bool TF_RemoveGenericListener(TF_Listener cb);

/**
 * Send a frame, no listener
 *
 * @param msg - message struct. ID is stored in the frame_id field
 * @return success
 */
bool TF_Send(TF_Msg *msg);

/**
 * Like TF_Send, but without the struct
 */
bool TF_SendSimple(TF_TYPE type, const uint8_t *data, TF_LEN len);

/**
 * Like TF_Query, but without the struct
 */
bool TF_QuerySimple(TF_TYPE type, const uint8_t *data, TF_LEN len, TF_Listener listener, TF_TICKS timeout, void *userdata);

/**
 * Send a frame, and optionally attach an ID listener.
 *
 * @param msg - message struct. ID is stored in the frame_id field
 * @param listener - listener waiting for the response (can be NULL)
 * @param timeout - listener expiry time in ticks
 * @return success
 */
bool TF_Query(TF_Msg *msg, TF_Listener listener, TF_TICKS timeout);

/**
 * Send a response to a received message.
 *
 * @param msg - message struct. ID is read from frame_id. set ->renew to reset listener timeout
 * @return success
 */
bool TF_Respond(TF_Msg *msg);

/**
 * Renew ID listener timeout
 *
 * @param id - listener ID to renew
 * @return true if listener was found and renewed
 */
bool TF_RenewIdListener(TF_ID id);

/**
 * Accept incoming bytes & parse frames
 *
 * @param buffer - byte buffer to process
 * @param count - nr of bytes in the buffer
 */
void TF_Accept(const uint8_t *buffer, size_t count);

/**
 * Accept a single incoming byte
 *
 * @param c - a received char
 */
void TF_AcceptChar(uint8_t c);

/**
 * 'Write bytes' function that sends data to UART
 *
 * ! Implement this in your application code !
 */
extern void TF_WriteImpl(const uint8_t *buff, size_t len);

/**
 * This function should be called periodically.
 *
 * The time base is used to time-out partial frames in the parser and
 * automatically reset it.
 *
 * (suggestion - call this in a SysTick handler)
 */
void TF_Tick(void);

/** Claim the TX interface before composing and sending a frame */
extern void TF_ClaimTx(void);

/** Free the TX interface after composing and sending a frame */
extern void TF_ReleaseTx(void);

#endif
