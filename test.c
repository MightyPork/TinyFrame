#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TinyFrame.h"

static void dumpFrame(const uint8_t *buff, TF_LEN len);

/**
 * This function should be defined in the application code.
 * It implements the lowest layer - sending bytes to UART (or other)
 */
void TF_WriteImpl(const uint8_t *buff, TF_LEN len)
{
	printf("--------------------\n");
	printf("\033[32mTF_WriteImpl - sending frame:\033[0m\n");
	dumpFrame(buff, len);

	// Send it back as if we received it
	TF_Accept(buff, len);
}

/** An example listener function */
bool myListener(TF_ID frame_id, TF_TYPE type, const uint8_t *buff, TF_LEN len)
{
	printf("\033[33mRX frame\n"
			   "  type: %02Xh\n"
			   "  data: \"%.*s\"\n"
			   "   len: %u\n"
			   "    id: %Xh\033[0m\n", type, len, buff, len, frame_id);
	return true;
}

bool testIdListener(TF_ID frame_id, TF_TYPE type, const uint8_t *buff, TF_LEN len)
{
	printf("OK - ID Listener triggered for msg (type %02X, id %Xh)!", type, frame_id);
	return true;
}

void main(void)
{
	// Set up the TinyFrame library
	TF_Init(TF_MASTER); // 1 = master, 0 = slave
	TF_AddGenericListener(myListener);

	printf("------ Simulate sending a message --------\n");

	TF_Send(0x22, (unsigned char*)"Hello TinyFrame", 16, NULL, NULL);

	const char *longstr = "Lorem ipsum dolor sit amet.";
	TF_Send(0x33, (unsigned char*)longstr, (TF_LEN)(strlen(longstr)+1), NULL, NULL);

	TF_Send(0x44, (unsigned char*)"Hello2", 7, NULL, NULL);

	TF_Send0(0xF0, NULL, NULL);

	TF_Send1(0xF1, 'Q', NULL, NULL);

	TF_Send2(0xF2, 'A', 'Z', NULL, NULL);

	TF_Send0(0x77, testIdListener, NULL);
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
