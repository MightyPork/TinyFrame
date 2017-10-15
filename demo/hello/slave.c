//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
#include "../../TinyFrame.h"
#include "../demo.h"
#include <unistd.h>
#include <memory.h>

bool helloListener(TF_MSG *msg)
{
	printf("helloListener()\n");
	dumpFrameInfo(msg);
	msg->data = (const uint8_t *) "jak se mas?";
	msg->len = (TF_LEN) strlen((const char *) msg->data);
	TF_Respond(msg);
	return true;
}

int main(void)
{
	TF_Init(TF_SLAVE);
	TF_AddTypeListener(1, helloListener);

	demo_init(TF_SLAVE);
	printf("MAIN PROCESS CONTINUES...\n");
	while(1) usleep(10);
}
