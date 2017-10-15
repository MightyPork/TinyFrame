#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TinyFrame.h"

typedef unsigned char* pu8;

static void dumpFrame(const uint8_t *buff, TF_LEN len);

/**
 * This function should be defined in the application code.
 * It implements the lowest layer - sending bytes to UART (or other)
 */
void TF_WriteImpl(const uint8_t *buff, size_t len)
{
	printf("--------------------\n");
	printf("\033[32mTF_WriteImpl - sending frame:\033[0m\n");
	dumpFrame(buff, len);

	// Send it back as if we received it
	TF_Accept(buff, len);
}

/** An example listener function */
bool myListener(TF_MSG *msg)
{
	printf("\033[33mRX frame\n"
			   "  type: %02Xh\n"
			   "  data: \"%.*s\"\n"
			   "   len: %u\n"
			   "    id: %Xh\033[0m\n",
			   msg->type, msg->len, msg->data, msg->len, msg->frame_id);
	return true;
}

bool testIdListener(TF_MSG *msg)
{
	printf("OK - ID Listener triggered for msg (type %02X, id %Xh)!",
		msg->type, msg->frame_id);
	return true;
}

void main(void)
{
	TF_MSG msg;
	const char *longstr = "Lorem ipsum dolor sit amet.";

	// Set up the TinyFrame library
	TF_Init(TF_MASTER); // 1 = master, 0 = slave
	TF_AddGenericListener(myListener);

	printf("------ Simulate sending a message --------\n");

	TF_ClearMsg(&msg);
	msg.type = 0x22;
	msg.data = (pu8)"Hello TinyFrame";
	msg.len = 16;
	TF_Send(&msg, NULL, 0);

	msg.type = 0x33;
	msg.data = (pu8)longstr;
	msg.len = (TF_LEN) (strlen(longstr)+1); // add the null byte
	TF_Send(&msg, NULL, 0);

	msg.type = 0x44;
	msg.data = (pu8)"Hello2";
	msg.len = 7;
	TF_Send(&msg, NULL, 0);

	msg.len = 0;
	msg.type = 0x77;
	TF_Send(&msg, testIdListener, 0);
}

// helper func for testing
static void dumpFrame(const uint8_t *buff, TF_LEN len)
{
	int i;
	for(i = 0; i < len; i++) {
		printf("%3u \033[34m%02X\033[0m", buff[i], buff[i]);
		if (buff[i] >= 0x20 && buff[i] < 127) {
			printf(" %c", buff[i]);
		} else {
			printf(" \033[31m.\033[0m");
		}
		printf("\n");
	}
	printf("--- end of frame ---\n");
}
