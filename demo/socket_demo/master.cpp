//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
#include "../demo.h"

Result testIdListener(TinyFrame *tf, Msg *msg)
{
    printf("testIdListener()\n");
    dumpFrameInfo(msg);
    return CLOSE;
}

Result testGenericListener(TinyFrame *tf, Msg *msg)
{
    printf("testGenericListener()\n");
    dumpFrameInfo(msg);
    return STAY;
}

int main(void)
{
    demo_tf = Init(MASTER);
    AddGenericListener(demo_tf, testGenericListener);

    demo_init(MASTER);

    SendSimple(demo_tf, 1, (pu8) "Ahoj", 5);
    SendSimple(demo_tf, 1, (pu8) "Hello", 6);

    QuerySimple(demo_tf, 2, (pu8) "Query!", 6, testIdListener, NULL, 0);

    demo_sleep();
}
