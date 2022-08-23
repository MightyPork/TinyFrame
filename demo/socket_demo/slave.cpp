//
// Created by MightyPork on 2017/10/15.
//

#include <stdio.h>
#include "../demo.h"

Result helloListener(TinyFrame *tf, Msg *msg)
{
    printf("helloListener()\n");
    dumpFrameInfo(msg);
    return STAY;
}

Result replyListener(TinyFrame *tf, Msg *msg)
{
    printf("replyListener()\n");
    dumpFrameInfo(msg);
    msg->data = (const uint8_t *) "response to query";
    msg->len = (LEN) strlen((const char *) msg->data);
    Respond(tf, msg);

    // unsolicited reply - will not be handled by the ID listener, which is already gone
    msg->data = (const uint8_t *) "SPAM";
    msg->len = 5;
    Respond(tf, msg);

    // unrelated message
    SendSimple(tf, 77, (const uint8_t *) "NAZDAR", 7);
    return STAY;
}

int main(void)
{
    demo_tf = Init(SLAVE);
    AddTypeListener(demo_tf, 1, helloListener);
    AddTypeListener(demo_tf, 2, replyListener);

    demo_init(SLAVE);
    demo_sleep();
}
