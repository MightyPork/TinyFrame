//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
#include "../demo.h"
#include <memory.h>

TF_Result helloListener(TF_Msg *msg)
{
	printf("helloListener()\n");
	dumpFrameInfo(msg);
	return TF_STAY;
}

TF_Result replyListener(TF_Msg *msg)
{
	printf("replyListener()\n");
	dumpFrameInfo(msg);
	msg->data = (const uint8_t *) "response to query";
	msg->len = (TF_LEN) strlen((const char *) msg->data);
	TF_Respond(msg);

	// unsolicited reply - will not be handled by the ID listener, which is already gone
	msg->data = (const uint8_t *) "SPAM";
	msg->len = 5;
	TF_Respond(msg);

	// unrelated message
	TF_SendSimple(77, (const uint8_t *) "NAZDAR", 7);
	return TF_STAY;
}

int main(void)
{
	TF_Init(TF_SLAVE);
	TF_AddTypeListener(1, helloListener);
	TF_AddTypeListener(2, replyListener);

	demo_init(TF_SLAVE);
	demo_sleep();
}
