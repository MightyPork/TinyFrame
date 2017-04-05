#ifndef TinyFrameH
#define TinyFrameH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
//---------------------------------------------------------------------------

// Frame ID listeners (wait for response / multi-part message)
#ifndef TF_MAX_ID_LST
# define TF_MAX_ID_LST   64
#endif

// Frame Type listeners (wait for frame with a specific first payload byte)
#ifndef TF_MAX_TYPE_LST
# define TF_MAX_TYPE_LST 16
#endif

// Generic listeners (fallback if no other listener catches it)
#ifndef TF_MAX_GEN_LST
# define TF_MAX_GEN_LST  4
#endif

#define TF_NEXT_ID -1
#define TF_ERROR -1

/**
 * TinyFrame receive callback type.
 * @param frame_id - ID of the received frame
 * @param buff - byte buffer with the payload
 * @param len - number of bytes in the buffer
 * @return true if the frame was consumed
 */
typedef bool (*TinyFrameListener)(unsigned int frame_id,
                                  const unsigned char *buff,
                                  unsigned int len);

/**
 * Initialize the TinyFrame engine.
 * This can also be used to completely reset it (removing all listeners etc)
 * @param peer_bit - peer bit to use for self
 */
void TF_Init(bool peer_bit);

/**
 * Reset the frame parser state machine.
 */
void TF_ResetParser(void);

/**
 * Accept incoming bytes & parse frames
 * @param buffer - byte buffer to process
 * @param count - nr of bytes in the buffer
 */
void TF_Accept(const unsigned char *buffer, unsigned int count);

/**
 * Accept a single incoming byte
 * @param c - a received char
 */
void TF_AcceptChar(unsigned char c);

/**
 * Register a frame type listener.
 * @param frame_type - frame ID to listen for
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
int TF_AddIdListener(unsigned int frame_id, TinyFrameListener cb);

/**
 * Register a frame type listener.
 * @param frame_type - frame type to listen for
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
int TF_AddTypeListener(unsigned char frame_type, TinyFrameListener cb);

/**
 * Register a generic listener.
 * @param cb - callback
 * @return slot index (for removing), or TF_ERROR (-1)
 */
int TF_AddGenericListener(TinyFrameListener cb);

/**
 * Remove any listener by the index received when registering it
 * @param index - index in the callbacks table
 */
void TF_RemoveListener(unsigned int index);

/**
 * Remove a listener by the message ID it's registered for
 * @param frame_id - the frame we're listening for
 */
void TF_RemoveIdListener(unsigned int frame_id);

/**
 * Remove a listener by type.
 * @param type - the type it's registered for
 */
void TF_RemoveTypeListener(unsigned char type);

/**
 * Remove a callback by the function pointer
 * @param cb - callback function to remove
 */
void TF_RemoveListenerFn(TinyFrameListener cb); // search all listener types

/**
 * Compose a frame
 *
 * @param outbuff - buffer to store the result in
 * @param msgid - message ID is stored here, if not NULL
 * @param payload - data buffer
 * @param len - payload size in bytes, 0 to use strlen
 * @param explicit_id - ID to use, TF_NEXT_ID (-1) to chose next free
 * @return nr of bytes in outbuff used by the frame, TF_ERROR (-1) on failure
 */
int TF_Compose(unsigned char *outbuff, unsigned int *msgid,
               const unsigned char *payload, unsigned int payload_len,
               int explicit_id);

/**
 * Send a frame, and optionally attach an ID listener for response.
 *
 * @param payload - data to send
 * @param payload_len - nr of bytes to send
 * @param id_ptr - store the ID here, NULL to don't store.
 *                 The ID may be used to unbind the listener after a timeout.
 * @return listener ID if listener was attached, else -1 (FT_ERROR).
 *         If the listener arg was NULL, returned -1 does not indicate an error.
 */
int TF_Send(const unsigned char *payload,
             unsigned int payload_len,
             TinyFrameListener listener, unsigned int *id_ptr);

/**
 * A shorthand for TF_Send() with just one byte as the payload
 */
int TF_Send1(const unsigned char b0,
             TinyFrameListener listener, unsigned int *id_ptr);

/**
 * A shorthand for TF_Send() with just 2 bytes as the payload
 */
int TF_Send2(const unsigned char b0,
             const unsigned char b1,
             TinyFrameListener listener, unsigned int *id_ptr);

/**
 * Send a response to a received message.
 *
 * @param payload - data to send
 * @param payload_len - nr of bytes to send
 * @param frame_id - ID of the original frame
 */
void TF_Respond(const unsigned char *payload,
               unsigned int payload_len,
               unsigned int frame_id);

/**
 * Write implementation for TF, to be provided by user code.
 * This sends frames composed by TinyFrame to UART
 */
extern void TF_WriteImpl(const unsigned char *buff, unsigned int len);

#endif
