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

#define TF_VERSION "2.0.0"

//---------------------------------------------------------------------------
#include <stdint.h>  // for uint8_t etc
#include <stdbool.h> // for bool
#include <stddef.h>  // for NULL
#include <string.h>  // for memset()
//---------------------------------------------------------------------------

// Select checksum type (0 = none, 8 = ~XOR, 16 = CRC16 0x8005, 32 = CRC32)
#define TF_CKSUM_NONE  0
#define TF_CKSUM_XOR   8
#define TF_CKSUM_CRC16 16
#define TF_CKSUM_CRC32 32

#include "TF_Config.h"

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
    TF_MASTER = 1,
} TF_Peer;

/** Response from listeners */
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
    memset(msg, 0, sizeof(TF_Msg));
}

typedef struct TinyFrame_ TinyFrame;

/**
 * TinyFrame Type Listener callback
 *
 * @param frame_id - ID of the received frame
 * @param type - type field from the message
 * @param data - byte buffer with the application data
 * @param len - number of bytes in the buffer
 * @return listener result
 */
typedef TF_Result (*TF_Listener)(TinyFrame *tf, TF_Msg *msg);

// -------------------------------------------------------------------

// region Internal

enum TFState_ {
    TFState_SOF = 0,      //!< Wait for SOF
    TFState_LEN,          //!< Wait for Number Of Bytes
    TFState_HEAD_CKSUM,   //!< Wait for header Checksum
    TFState_ID,           //!< Wait for ID
    TFState_TYPE,         //!< Wait for message type
    TFState_DATA,         //!< Receive payload
    TFState_DATA_CKSUM    //!< Wait for Checksum
};

struct TF_IdListener_ {
    TF_ID id;
    TF_Listener fn;
    TF_TICKS timeout;     // nr of ticks remaining to disable this listener
    TF_TICKS timeout_max; // the original timeout is stored here
    void *userdata;
    void *userdata2;
};

struct TF_TypeListener_ {
    TF_TYPE type;
    TF_Listener fn;
};

struct TF_GenericListener_ {
    TF_Listener fn;
};

/**
 * Frame parser internal state.
 */
struct TinyFrame_ {
    /* Public user data */
    void *userdata;
    uint32_t usertag;

    // --- the rest of the struct is internal, do not access directly ---

    /* Own state */
    TF_Peer peer_bit;       //!< Own peer bit (unqiue to avoid msg ID clash)
    TF_ID next_id;          //!< Next frame / frame chain ID

    /* Parser state */
    enum TFState_ state;
    TF_TICKS parser_timeout_ticks;
    TF_ID id;               //!< Incoming packet ID
    TF_LEN len;             //!< Payload length
    uint8_t data[TF_MAX_PAYLOAD_RX]; //!< Data byte buffer
    TF_LEN rxi;             //!< Field size byte counter
    TF_CKSUM cksum;         //!< Checksum calculated of the data stream
    TF_CKSUM ref_cksum;     //!< Reference checksum read from the message
    TF_TYPE type;           //!< Collected message type number
    bool discard_data;      //!< Set if (len > TF_MAX_PAYLOAD) to read the frame, but ignore the data.

    /* --- Callbacks --- */

    /* Transaction callbacks */
    struct TF_IdListener_ id_listeners[TF_MAX_ID_LST];
    struct TF_TypeListener_ type_listeners[TF_MAX_TYPE_LST];
    struct TF_GenericListener_ generic_listeners[TF_MAX_GEN_LST];

    // Those counters are used to optimize look-up times.
    // They point to the highest used slot number,
    // or close to it, depending on the removal order.
    TF_COUNT count_id_lst;
    TF_COUNT count_type_lst;
    TF_COUNT count_generic_lst;

    // Buffer for building frames
    uint8_t sendbuf[TF_SENDBUF_LEN];
};

// endregion

// -------------------------------------------------------------------

/**
 * Initialize the TinyFrame engine.
 * This can also be used to completely reset it (removing all listeners etc).
 *
 * The field .userdata (or .usertag) can be used to identify different instances
 * in the TF_WriteImpl() function etc. Set this field after the init.
 *
 * This function is a wrapper around TF_InitStatic that calls malloc() to obtain
 * the instance.
 *
 * @param peer_bit - peer bit to use for self
 */
TinyFrame *TF_Init(TF_Peer peer_bit);


/**
 * Initialize the TinyFrame engine using a statically allocated instance struct.
 *
 * The .userdata / .usertag field is preserved when TF_InitStatic is called.
 *
 * @param peer_bit - peer bit to use for self
 */
void TF_InitStatic(TinyFrame *tf, TF_Peer peer_bit);

/**
 * Reset the frame parser state machine.
 * This does not affect registered listeners.
 */
void TF_ResetParser(TinyFrame *tf);

/**
 * Register a frame type listener.
 *
 * @param msg - message (contains frame_id and userdata)
 * @param cb - callback
 * @param timeout - timeout in ticks to auto-remove the listener (0 = keep forever)
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddIdListener(TinyFrame *tf, TF_Msg *msg, TF_Listener cb, TF_TICKS timeout);

/**
 * Remove a listener by the message ID it's registered for
 *
 * @param frame_id - the frame we're listening for
 */
bool TF_RemoveIdListener(TinyFrame *tf, TF_ID frame_id);

/**
 * Register a frame type listener.
 *
 * @param frame_type - frame type to listen for
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddTypeListener(TinyFrame *tf, TF_TYPE frame_type, TF_Listener cb);

/**
 * Remove a listener by type.
 *
 * @param type - the type it's registered for
 */
bool TF_RemoveTypeListener(TinyFrame *tf, TF_TYPE type);

/**
 * Register a generic listener.
 *
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddGenericListener(TinyFrame *tf, TF_Listener cb);

/**
 * Remove a generic listener by function pointer
 *
 * @param cb - callback function to remove
 */
bool TF_RemoveGenericListener(TinyFrame *tf, TF_Listener cb);

/**
 * Send a frame, no listener
 *
 * @param msg - message struct. ID is stored in the frame_id field
 * @return success
 */
bool TF_Send(TinyFrame *tf, TF_Msg *msg);

/**
 * Like TF_Send, but without the struct
 */
bool TF_SendSimple(TinyFrame *tf, TF_TYPE type, const uint8_t *data, TF_LEN len);

/**
 * Send a frame, and optionally attach an ID listener.
 *
 * @param msg - message struct. ID is stored in the frame_id field
 * @param listener - listener waiting for the response (can be NULL)
 * @param timeout - listener expiry time in ticks
 * @return success
 */
bool TF_Query(TinyFrame *tf, TF_Msg *msg, TF_Listener listener, TF_TICKS timeout);

/**
 * Like TF_Query, but without the struct
 */
bool TF_QuerySimple(TinyFrame *tf, TF_TYPE type, const uint8_t *data, TF_LEN len,
                    TF_Listener listener, TF_TICKS timeout);

/**
 * Send a response to a received message.
 *
 * @param msg - message struct. ID is read from frame_id. set ->renew to reset listener timeout
 * @return success
 */
bool TF_Respond(TinyFrame *tf, TF_Msg *msg);

/**
 * Renew an ID listener timeout externally (as opposed to by returning TF_RENEW from the ID listener)
 *
 * @param id - listener ID to renew
 * @return true if listener was found and renewed
 */
bool TF_RenewIdListener(TinyFrame *tf, TF_ID id);

/**
 * Accept incoming bytes & parse frames
 *
 * @param buffer - byte buffer to process
 * @param count - nr of bytes in the buffer
 */
void TF_Accept(TinyFrame *tf, const uint8_t *buffer, size_t count);

/**
 * Accept a single incoming byte
 *
 * @param c - a received char
 */
void TF_AcceptChar(TinyFrame *tf, uint8_t c);

/**
 * This function should be called periodically.
 *
 * The time base is used to time-out partial frames in the parser and
 * automatically reset it.
 *
 * (suggestion - call this in a SysTick handler)
 */
void TF_Tick(TinyFrame *tf);

// --- TO BE IMPLEMENTED BY USER ---

/**
 * 'Write bytes' function that sends data to UART
 *
 * ! Implement this in your application code !
 */
extern void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, size_t len);

/** Claim the TX interface before composing and sending a frame */
extern void TF_ClaimTx(TinyFrame *tf);

/** Free the TX interface after composing and sending a frame */
extern void TF_ReleaseTx(TinyFrame *tf);

#endif
