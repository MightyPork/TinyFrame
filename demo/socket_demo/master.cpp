//
// Created by MightyPork on 2017/10/15.
//

#include <cstdio>
#include "../demo.hpp"

using namespace TinyFrame_n;
using TinyFrame_Demo=TinyFrameDefault;

Result testIdListener(Msg *msg)
{
    printf("testIdListener()\n");
    dumpFrameInfo(msg);
    return Result::CLOSE;
}

Result testGenericListener(Msg *msg)
{
    printf("testGenericListener()\n");
    dumpFrameInfo(msg);
    return Result::STAY;
}

int main(void)
{
    demo_tf = new TinyFrame_Demo({.WriteImpl=&WriteImpl, .Error=&ErrorCallback}, Peer::MASTER); // 1 = master, 0 = slave
    demo_tf->AddGenericListener(testGenericListener);

    demo_init(Peer::MASTER);

    demo_tf->SendSimple(1, (pu8) "Ahoj", 5);
    demo_tf->SendSimple(1, (pu8) "Hello", 6);

    demo_tf->QuerySimple(2, (pu8) "Query!", 6, testIdListener, NULL, 0);

    demo_sleep();
}
