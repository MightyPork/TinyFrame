//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>
#include "../../TinyFrame.h"
#include "../demo.h"

bool testIdListener(TF_MSG *msg)
{
	printf("testIdListener()\n");
	dumpFrameInfo(msg);
	return true;
}

bool testGenericListener(TF_MSG *msg)
{
	printf("testGenericListener()\n");
	dumpFrameInfo(msg);
	return true;
}

int main(void)
{
	TF_Init(TF_MASTER);
	TF_AddGenericListener(testGenericListener);

	demo_init(TF_MASTER);

	TF_SendSimple(1, (pu8)"Ahoj", 5);
	TF_SendSimple(1, (pu8)"Hello", 6);

	TF_QuerySimple(1, (pu8)"Query!", 6, testIdListener, 0);

	while(1) usleep(10);
}
