//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
#include "../demo.h"

TF_Result testIdListener(TinyFrame *tf, TF_Msg *msg)
{
    printf("testIdListener()\n");
    dumpFrameInfo(msg);
    return TF_CLOSE;
}

TF_Result testGenericListener(TinyFrame *tf, TF_Msg *msg)
{
    printf("testGenericListener()\n");
    dumpFrameInfo(msg);
    return TF_STAY;
}

int main(void)
{
    demo_tf = TF_Init(TF_MASTER);
    TF_AddGenericListener(demo_tf, testGenericListener);

    demo_init(TF_MASTER);

    TF_SendSimple(demo_tf, 1, (pu8) "Ahoj", 5);
    TF_SendSimple(demo_tf, 1, (pu8) "Hello", 6);

    TF_QuerySimple(demo_tf, 2, (pu8) "Query!", 6, testIdListener, 0);

    demo_sleep();
}
