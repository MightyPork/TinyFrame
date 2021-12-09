#ifndef TinyFrameH
#define TinyFrameH

/**
 * TinyFrame protocol library
 *
 * (c) Ondřej Hruška 2017-2018, MIT License
 * no liability/warranty, free for any use, must retain this notice & license
 *
 * Upstream URL: https://github.com/MightyPork/TinyFrame
 */

#define TF_VERSION "2.3.0"

//---------------------------------------------------------------------------
#include <stdint.h>  // for uint8_t etc
#include <stdbool.h> // for bool
#include <stddef.h>  // for NULL
#include <string.h>  // for memset()
//---------------------------------------------------------------------------

// Checksum type (0 = none, 8 = ~XOR, 16 = CRC16 0x8005, 32 = CRC32)
#define TF_CKSUM_NONE  0  // no checksums
#define TF_CKSUM_XOR   8  // inverted xor of all payload bytes
#define TF_CKSUM_CRC8  9  // Dallas/Maxim CRC8 (1-wire)
#define TF_CKSUM_CRC16 16 // CRC16 with the polynomial 0x8005 (x^16 + x^15 + x^2 + 1)
#define TF_CKSUM_CRC32 32 // CRC32 with the polynomial 0xedb88320
#define TF_CKSUM_CUSTOM8  1  // Custom 8-bit checksum
#define TF_CKSUM_CUSTOM16 2  // Custom 16-bit checksum
#define TF_CKSUM_CUSTOM32 3  // Custom 32-bit checksum

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


#if (TF_CKSUM_TYPE == TF_CKSUM_XOR) || (TF_CKSUM_TYPE == TF_CKSUM_NONE) || (TF_CKSUM_TYPE == TF_CKSUM_CUSTOM8) || (TF_CKSUM_TYPE == TF_CKSUM_CRC8)
    // ~XOR (if 0, still use 1 byte - it won't be used)
    typedef uint8_t TF_CKSUM;
#elif (TF_CKSUM_TYPE == TF_CKSUM_CRC16) || (TF_CKSUM_TYPE == TF_CKSUM_CUSTOM16)
    // CRC16
    typedef uint16_t TF_CKSUM;
#elif (TF_CKSUM_TYPE == TF_CKSUM_CRC32) || (TF_CKSUM_TYPE == TF_CKSUM_CUSTOM32)
    // CRC32
    typedef uint32_t TF_CKSUM;
#else
    #error Bad value for TF_CKSUM_TYPE
#endif

//endregion

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
typedef struct TF_Msg_ {
    TF_ID frame_id;       //!< message ID
    bool is_response;     //!< internal flag, set when using the Respond function. frame_id is then kept unchanged.
    TF_TYPE type;         //!< received or sent message type

    /**
     * Buffer of received data, or data to send.
     *
     * - If (data == NULL) in an ID listener, that means the listener timed out and
     *   the user should free any userdata and take other appropriate actions.
     *
     * - If (data == NULL) and length is not zero when sending a frame, that starts a multi-part frame.
     *   This call then must be followed by sending the payload and closing the frame.
     */
    const uint8_t *data;
    TF_LEN len; //!< length of the payload

    /**
     * Custom user data for the ID listener.
     *
     * This data will be stored in the listener slot and passed to the ID callback
     * via those same fields on the received message.
     */
    void *userdata;
    void *userdata2;
} TF_Msg;

/**
 * Clear message struct
 *
 * @param msg - message to clear in-place
 */
static inline void TF_ClearMsg(TF_Msg *msg)
{
    memset(msg, 0, sizeof(TF_Msg));
}

/** TinyFrame struct typedef */
typedef struct TinyFrame_ TinyFrame;

/**
 * TinyFrame Type Listener callback
 *
 * @param tf - instance
 * @param msg - the received message, userdata is populated inside the object
 * @return listener result
 */
typedef TF_Result (*TF_Listener)(TinyFrame *tf, TF_Msg *msg);

/**
 * TinyFrame Type Listener callback
 *
 * @param tf - instance
 * @param msg - the received message, userdata is populated inside the object
 * @return listener result
 */
typedef TF_Result (*TF_Listener_Timeout)(TinyFrame *tf);

// ---------------------------------- INIT ------------------------------

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
 * @param tf - instance
 * @param peer_bit - peer bit to use for self
 * @return TF instance or NULL
 */
TinyFrame *TF_Init(TF_Peer peer_bit);


/**
 * Initialize the TinyFrame engine using a statically allocated instance struct.
 *
 * The .userdata / .usertag field is preserved when TF_InitStatic is called.
 *
 * @param tf - instance
 * @param peer_bit - peer bit to use for self
 * @return success
 */
bool TF_InitStatic(TinyFrame *tf, TF_Peer peer_bit);

/**
 * De-init the dynamically allocated TF instance
 *
 * @param tf - instance
 */
void TF_DeInit(TinyFrame *tf);


// ---------------------------------- API CALLS --------------------------------------

/**
 * Accept incoming bytes & parse frames
 *
 * @param tf - instance
 * @param buffer - byte buffer to process
 * @param count - nr of bytes in the buffer
 */
void TF_Accept(TinyFrame *tf, const uint8_t *buffer, uint32_t count);

/**
 * Accept a single incoming byte
 *
 * @param tf - instance
 * @param c - a received char
 */
void TF_AcceptChar(TinyFrame *tf, uint8_t c);

/**
 * This function should be called periodically.
 * The time base is used to time-out partial frames in the parser and
 * automatically reset it.
 * It's also used to expire ID listeners if a timeout is set when registering them.
 *
 * A common place to call this from is the SysTick handler.
 *
 * @param tf - instance
 */
void TF_Tick(TinyFrame *tf);

/**
 * Reset the frame parser state machine.
 * This does not affect registered listeners.
 *
 * @param tf - instance
 */
void TF_ResetParser(TinyFrame *tf);


// ---------------------------- MESSAGE LISTENERS -------------------------------

/**
 * Register a frame type listener.
 *
 * @param tf - instance
 * @param msg - message (contains frame_id and userdata)
 * @param cb - callback
 * @param ftimeout - time out callback
 * @param timeout - timeout in ticks to auto-remove the listener (0 = keep forever)
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddIdListener(TinyFrame *tf, TF_Msg *msg, TF_Listener cb, TF_Listener_Timeout ftimeout, TF_TICKS timeout);

/**
 * Remove a listener by the message ID it's registered for
 *
 * @param tf - instance
 * @param frame_id - the frame we're listening for
 */
bool TF_RemoveIdListener(TinyFrame *tf, TF_ID frame_id);

/**
 * Register a frame type listener.
 *
 * @param tf - instance
 * @param frame_type - frame type to listen for
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddTypeListener(TinyFrame *tf, TF_TYPE frame_type, TF_Listener cb);

/**
 * Remove a listener by type.
 *
 * @param tf - instance
 * @param type - the type it's registered for
 */
bool TF_RemoveTypeListener(TinyFrame *tf, TF_TYPE type);

/**
 * Register a generic listener.
 *
 * @param tf - instance
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
bool TF_AddGenericListener(TinyFrame *tf, TF_Listener cb);

/**
 * Remove a generic listener by function pointer
 *
 * @param tf - instance
 * @param cb - callback function to remove
 */
bool TF_RemoveGenericListener(TinyFrame *tf, TF_Listener cb);

/**
 * Renew an ID listener timeout externally (as opposed to by returning TF_RENEW from the ID listener)
 *
 * @param tf - instance
 * @param id - listener ID to renew
 * @return true if listener was found and renewed
 */
bool TF_RenewIdListener(TinyFrame *tf, TF_ID id);


// ---------------------------- FRAME TX FUNCTIONS ------------------------------

/**
 * Send a frame, no listener
 *
 * @param tf - instance
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
 * @param tf - instance
 * @param msg - message struct. ID is stored in the frame_id field
 * @param listener - listener waiting for the response (can be NULL)
 * @param ftimeout - time out callback
 * @param timeout - listener expiry time in ticks
 * @return success
 */
bool TF_Query(TinyFrame *tf, TF_Msg *msg, TF_Listener listener,
              TF_Listener_Timeout ftimeout, TF_TICKS timeout);

/**
 * Like TF_Query(), but without the struct
 */
bool TF_QuerySimple(TinyFrame *tf, TF_TYPE type,
                    const uint8_t *data, TF_LEN len,
                    TF_Listener listener, TF_Listener_Timeout ftimeout, TF_TICKS timeout);

/**
 * Send a response to a received message.
 *
 * @param tf - instance
 * @param msg - message struct. ID is read from frame_id. set ->renew to reset listener timeout
 * @return success
 */
bool TF_Respond(TinyFrame *tf, TF_Msg *msg);


// ------------------------ MULTIPART FRAME TX FUNCTIONS -----------------------------
// Those routines are used to send long frames without having all the data available
// at once (e.g. capturing it from a peripheral or reading from a large memory buffer)

/**
 * TF_Send() with multipart payload.
 * msg.data is ignored and set to NULL
 */
bool TF_Send_Multipart(TinyFrame *tf, TF_Msg *msg);

/**
 * TF_SendSimple() with multipart payload.
 */
bool TF_SendSimple_Multipart(TinyFrame *tf, TF_TYPE type, TF_LEN len);

/**
 * TF_QuerySimple() with multipart payload.
 */
bool TF_QuerySimple_Multipart(TinyFrame *tf, TF_TYPE type, TF_LEN len, TF_Listener listener, TF_Listener_Timeout ftimeout, TF_TICKS timeout);

/**
 * TF_Query() with multipart payload.
 * msg.data is ignored and set to NULL
 */
bool TF_Query_Multipart(TinyFrame *tf, TF_Msg *msg, TF_Listener listener, TF_Listener_Timeout ftimeout, TF_TICKS timeout);

/**
 * TF_Respond() with multipart payload.
 * msg.data is ignored and set to NULL
 */
void TF_Respond_Multipart(TinyFrame *tf, TF_Msg *msg);

/**
 * Send the payload for a started multipart frame. This can be called multiple times
 * if needed, until the full length is transmitted.
 *
 * @param tf - instance
 * @param buff - buffer to send bytes from
 * @param length - number of bytes to send
 */
void TF_Multipart_Payload(TinyFrame *tf, const uint8_t *buff, uint32_t length);

/**
 * Close the multipart message, generating chekcsum and releasing the Tx lock.
 *
 * @param tf - instance
 */
void TF_Multipart_Close(TinyFrame *tf);


// ---------------------------------- INTERNAL ----------------------------------
// This is publicly visible only to allow static init.

enum TF_State_ {
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
    TF_Listener_Timeout fn_timeout;
    TF_TICKS timeout;     // nr of ticks remaining to disable this listener
    TF_TICKS timeout_max; // the original timeout is stored here (0 = no timeout)
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
    enum TF_State_ state;
    TF_TICKS parser_timeout_ticks;
    TF_ID id;               //!< Incoming packet ID
    TF_LEN len;             //!< Payload length
    uint8_t data[TF_MAX_PAYLOAD_RX]; //!< Data byte buffer
    TF_LEN rxi;             //!< Field size byte counter
    TF_CKSUM cksum;         //!< Checksum calculated of the data stream
    TF_CKSUM ref_cksum;     //!< Reference checksum read from the message
    TF_TYPE type;           //!< Collected message type number
    bool discard_data;      //!< Set if (len > TF_MAX_PAYLOAD) to read the frame, but ignore the data.

    /* Tx state */
    // Buffer for building frames
    uint8_t sendbuf[TF_SENDBUF_LEN]; //!< Transmit temporary buffer

    uint32_t tx_pos;        //!< Next write position in the Tx buffer (used for multipart)
    uint32_t tx_len;        //!< Total expected Tx length
    TF_CKSUM tx_cksum;      //!< Transmit checksum accumulator

#if !TF_USE_MUTEX
    bool soft_lock;         //!< Tx lock flag used if the mutex feature is not enabled.
#endif

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
};


// ------------------------ TO BE IMPLEMENTED BY USER ------------------------

/**
 * 'Write bytes' function that sends data to UART
 *
 * ! Implement this in your application code !
 */
extern void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len);

// Mutex functions
#if TF_USE_MUTEX

    /** Claim the TX interface before composing and sending a frame */
    extern bool TF_ClaimTx(TinyFrame *tf);

    /** Free the TX interface after composing and sending a frame */
    extern void TF_ReleaseTx(TinyFrame *tf);

#endif

// Custom checksum functions
#if (TF_CKSUM_TYPE == TF_CKSUM_CUSTOM8) || (TF_CKSUM_TYPE == TF_CKSUM_CUSTOM16) || (TF_CKSUM_TYPE == TF_CKSUM_CUSTOM32)

    /**
     * Initialize a checksum
     *
     * @return initial checksum value
     */
    extern TF_CKSUM TF_CksumStart(void);

    /**
     * Update a checksum with a byte
     *
     * @param cksum - previous checksum value
     * @param byte - byte to add
     * @return updated checksum value
     */
    extern TF_CKSUM TF_CksumAdd(TF_CKSUM cksum, uint8_t byte);

    /**
     * Finalize the checksum calculation
     *
     * @param cksum - previous checksum value
     * @return final checksum value
     */
    extern TF_CKSUM TF_CksumEnd(TF_CKSUM cksum);

#endif

#endif
