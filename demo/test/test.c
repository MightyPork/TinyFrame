#include <stdio.h>
#include <string.h>
#include "../../TinyFrame.h"
#include "../utils.h"


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
	dumpFrameInfo(msg);
	return true;
}

bool testIdListener(TF_MSG *msg)
{
	printf("OK - ID Listener triggered for msg!\n");
	dumpFrameInfo(msg);
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
	TF_Send(&msg);

	msg.type = 0x33;
	msg.data = (pu8)longstr;
	msg.len = (TF_LEN) (strlen(longstr)+1); // add the null byte
	TF_Send(&msg);

	msg.type = 0x44;
	msg.data = (pu8)"Hello2";
	msg.len = 7;
	TF_Send(&msg);

	msg.len = 0;
	msg.type = 0x77;
	TF_Query(&msg, testIdListener, 0);
}
