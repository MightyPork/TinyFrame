#include <stdio.h>
#include <stdlib.h>
#include "TinyFrame.h"

// helper func for testing
static void dumpFrame(const uint8_t *buff, TF_LEN len)
{
	int i;
	for(i = 0; i < len; i++) {
		printf("%3u \033[34m%02X\033[0m", buff[i], buff[i]);
		if (buff[i] >= 0x20 && buff[i] < 127) {
			printf(" %c", buff[i]);
		} else {
			printf(" \033[31m.\033[0m", buff[i]);
		}
		printf("\n");
	}
	printf("--- end of frame ---\n");
}

/**
 * This function should be defined in the application code.
 * It implements the lowest layer - sending bytes to UART (or other)
 */
void TF_WriteImpl(const uint8_t *buff, TF_LEN len)
{
	printf("\033[32;1mTF_WriteImpl - sending frame:\033[0m\n");
	dumpFrame(buff, len);
}

/** An example listener function */
bool myListener(TF_ID frame_id, TF_TYPE type, const uint8_t *buff, TF_LEN len)
{
	printf("\033[33mrx frame %s, len %d, id %d\033[0m\n", buff, len, frame_id);
	return true;
}

void main()
{
	int i;
	TF_ID msgid;
	int len;
	char buff[100];

	// Set up the TinyFrame library
	TF_Init(TF_MASTER); // 1 = master, 0 = slave

	printf("------ Simulate sending a message --------\n");

	// Send a message
	//   args - payload, length (0 = strlen), listener, id_ptr (for storing the frame ID)
	//   (see the .h file for details)
	TF_Respond(0xA5, (unsigned char*)"Hello TinyFrame", 16, 0x15);
	// This builds the frame in an internal buffer and sends it to
	//   TF_WriteImpl()


	printf("------ Simulate receiving a message --------\n");

	// Adding global listeners
	//   Listeners can listen to any frame (fallback listeners),
	//   or to a specific Frame Type (AddTypeListener). There are
	//   also ID listeners that can be bound automatically in TF_Send().
	//
	//   Type listeners are matched by the first character of the payload,
	//   ID listeners by the message ID (which is the same in the response as in the request)
	TF_AddGenericListener(myListener);
	//TF_AddTypeListener(0xF1, myTypeListener);
	//TF_AddIdListener(msgID, myIdListener);

	// This lets us compose a frame (it's also used internally by TF_Send and TF_Respond)
	len = TF_Compose((unsigned char*)buff,          // Buffer to write the frame to
					 &msgid,        // Int to store the message ID in
					 0xA5,
					 (unsigned char*)"Hello TinyFrame",  // Payload bytes
	                 16,             // Length - this will cut it at "ALPHA" (showing that it works)
	                                //   For string, we can use "0" to use strlen() internally
	                 0x15, true);   // Message ID - we could specify a particular ID if we were
	                                //   trying to build a response frame, which has the same ID
	                                //   as the request it responds to.

	printf("The frame we'll receive is:\n");
	dumpFrame((unsigned char*)buff, (TF_LEN)len);

	// Accept the frame
	// You will normally call this method in the UART IRQ handler etc
	TF_Accept((unsigned char*)buff, (TF_LEN)len);
}
