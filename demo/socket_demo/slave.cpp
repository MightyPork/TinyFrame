//
// Created by MightyPork on 2017/10/15.
//

#include <cstdio>
#include <cstring>
#include "../demo.hpp"

using namespace TinyFrame_n;
using TinyFrame_Demo=TinyFrame<>;

Result helloListener(Msg *msg)
{
    printf("helloListener()\n");
    dumpFrameInfo(msg);
    return Result::STAY;
}

Result replyListener(Msg *msg)
{
    printf("replyListener()\n");
    dumpFrameInfo(msg);
    msg->data = (const uint8_t *) "response to query";
    msg->len = (LEN) strlen((const char *) msg->data);
    demo_tf->Respond(msg);

    // unsolicited reply - will not be handled by the ID listener, which is already gone
    msg->data = (const uint8_t *) "SPAM";
    msg->len = 5;
    demo_tf->Respond(msg);

    // unrelated message
    demo_tf->SendSimple(77, (const uint8_t *) "NAZDAR", 7);
    return Result::STAY;
}

int main(void)
{
    demo_tf = new TinyFrame_Demo({.WriteImpl=&WriteImpl, .Error=&ErrorCallback}, Peer::SLAVE); // 1 = master, 0 = slave
    demo_tf->AddTypeListener(1, helloListener);
    demo_tf->AddTypeListener(2, replyListener);

    demo_init(Peer::SLAVE);
    demo_sleep();
}
