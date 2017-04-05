#include <stdio.h>
#include <stdlib.h>
#include "TinyFrame.h"

// helper func for testing
static void dumpFrame(const unsigned char *buff, unsigned int len)
{
	int i;
	for(i = 0; i < len; i++) {
		printf("%3u %c\n", buff[i], buff[i]);
	}
	printf("--- end of frame ---\n");
}

/**
 * This function should be defined in the application code.
 * It implements the lowest layer - sending bytes to UART (or other)
 */
void TF_WriteImpl(const unsigned char *buff, unsigned int len)
{
	printf("\033[32;1mTF_WriteImpl - sending frame:\033[0m\n");
	dumpFrame(buff, len);
}

/** An example listener function */
bool myListener(unsigned int frame_id, const unsigned char *buff, unsigned int len)
{
	printf("\033[33mrx frame %s, len %d, id %d\033[0m\n", buff, len, frame_id);

	return false; // Do not unbind
}

void main()
{
	int i;
	int msgid;
	int len;
	char buff[100];

	// Set up the TinyFrame library
	TF_Init(1); // 1 = master, 0 = slave

	printf("------ Simulate sending a message --------\n");

	// Send a message, not expecting any reply
	// Returns Listener ID (if the listener is not NULL, like here)
	TF_Send("Hello TinyFrame", 0, NULL);
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
	len = TF_Compose(buff,          // Buffer to write the frame to
	                 &msgid,        // Int to store the message ID in
	                 "ALPHA BETA",  // Payload bytes
	                 5,             // Length - this will cut it at "ALPHA" (showing that it works)
	                                //   For string, we can use "0" to use strlen() internally
	                 TF_NEXT_ID);   // Message ID - we could specify a particular ID if we were
	                                //   trying to build a response frame, which has the same ID
	                                //   as the request it responds to.

	printf("The frame we'll receive is:\n");
	dumpFrame(buff, len);

	// Accept the frame
	// You will normally call this method in the UART IRQ handler etc
	TF_Accept(buff, len);
}
