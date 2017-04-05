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

/**
 * TinyFrame receive callback type.
 *
 * @param frame_id - ID of the received frame
 * @param buff - byte buffer with the payload
 * @param len - number of bytes in the buffer
 * @return true if the frame was consumed
 */
typedef bool (*TinyFrameListener)(
	unsigned int frame_id,
	const unsigned char *buff,
	unsigned int len
);

// Forward declarations (comments in .c)

void TF_ResetParser(void);
void TF_Init(bool peer_bit);

void TF_Accept(const unsigned char *buffer, unsigned int count);
void TF_AcceptChar(unsigned char c);

// Add?Listener all return a unique listener ID
int TF_AddIdListener(unsigned int frame_id, TinyFrameListener cb);
int TF_AddTypeListener(unsigned char frame_type, TinyFrameListener cb);
int TF_AddGenericListener(TinyFrameListener cb);

void TF_RemoveListener(unsigned int index);
void TF_RemoveIdListener(unsigned int frame_id);
void TF_RemoveTypeListener(unsigned char type);
void TF_RemoveListenerFn(TinyFrameListener cb); // search all listener types

#define TF_NEXT_ID -1
#define TF_ERROR -1

// returns nr of bytes in the output buffer used, or TF_ERROR
int TF_Compose(unsigned char *outbuff, unsigned int *msgid,
			   const unsigned char *payload, unsigned int payload_len,
			   int explicit_id);

// returns listener ID (-1 if listener is NULL - does not indicate error)
int TF_Send(const unsigned char *payload,
			 unsigned int payload_len,
			 TinyFrameListener listener);

int TF_Send1(const unsigned char b0,
			 TinyFrameListener listener);

int TF_Send2(const unsigned char b0,
			 const unsigned char b1,
			 TinyFrameListener listener);

// Respond with the same ID
void TF_Respond(const unsigned char *payload,
			   unsigned int payload_len,
			   int frame_id);

/** Write implementation for TF, to be provided by user code */
extern void TF_WriteImpl(const unsigned char *buff, unsigned int len);

#endif
