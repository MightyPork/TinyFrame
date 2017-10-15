//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
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

	TF_QuerySimple(2, (pu8)"Query!", 6, testIdListener, 0);

	demo_sleep();
}
