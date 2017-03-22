#ifndef TinyFrameH
#define TinyFrameH
//---------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
//---------------------------------------------------------------------------

/**
 * TinyFrame receive callback type.
 *
 * @param frame_id - ID of the received byte (if response, same as the request)
 * @param buff - byte buffer with the payload
 * @param len - number of bytes in the buffer
 * @return return true, if the callback should be removed.
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

#define TF_ANY_ID -1
int TF_AddListener(int frame_id, TinyFrameListener cb); // returns slot index
void TF_RemoveListener(int index);
void TF_RemoveListenerFn(TinyFrameListener cb);

// returns "frame_id"

#define TF_NEXT_ID -1
#define TF_ERROR -1

// returns nr of bytes in the output buffer used, or TF_ERROR
int TF_Compose(unsigned char *outbuff, unsigned int *msgid,
               const unsigned char *payload, unsigned int payload_len,
               int explicit_id
);

#endif
