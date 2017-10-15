//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
#include "../demo.h"
#include <memory.h>

bool helloListener(TF_MSG *msg)
{
	printf("helloListener()\n");
	dumpFrameInfo(msg);
	return true;
}

bool replyListener(TF_MSG *msg)
{
	printf("replyListener()\n");
	dumpFrameInfo(msg);
	msg->data = (const uint8_t *) "response to query";
	msg->len = (TF_LEN) strlen((const char *) msg->data);
	TF_Respond(msg);

	TF_SendSimple(77, (const uint8_t *) "NAZDAR", 7);
	return true;
}

int main(void)
{
	TF_Init(TF_SLAVE);
	TF_AddTypeListener(1, helloListener);
	TF_AddTypeListener(2, replyListener);

	demo_init(TF_SLAVE);
	demo_sleep();
}
