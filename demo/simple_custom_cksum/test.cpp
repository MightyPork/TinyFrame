#include <stdio.h>
#include <string.h>
#include "../../TinyFrame.hpp"
#include "../utils.hpp"

TinyFrame *demo_tf;

bool do_corrupt = false;

/**
 * This function should be defined in the application code.
 * It implements the lowest layer - sending bytes to UART (or other)
 */
void WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len)
{
    printf("--------------------\n");
    printf("\033[32mWriteImpl - sending frame:\033[0m\n");
    
    uint8_t *xbuff = (uint8_t *)buff;    
    if (do_corrupt) {
      printf("(corrupting to test checksum checking...)\n");
      xbuff[8]++;
    }
    
    dumpFrame(xbuff, len);

    // Send it back as if we received it
    Accept(tf, xbuff, len);
}

/** An example listener function */
Result myListener(TinyFrame *tf, Msg *msg)
{
    dumpFrameInfo(msg);
    return STAY;
}

Result testIdListener(TinyFrame *tf, Msg *msg)
{
    printf("OK - ID Listener triggered for msg!\n");
    dumpFrameInfo(msg);
    return CLOSE;
}

void main(void)
{
    Msg msg;
    const char *longstr = "Lorem ipsum dolor sit amet.";

    // Set up the TinyFrame library
    demo_tf = Init(MASTER); // 1 = master, 0 = slave
    AddGenericListener(demo_tf, myListener);

    printf("------ Simulate sending a message --------\n");

    ClearMsg(&msg);
    msg.type = 0x22;
    msg.data = (pu8) "Hello TinyFrame";
    msg.len = 16;
    Send(demo_tf, &msg);
    
    printf("This should fail:\n");
    
    // test checksums are tested
    do_corrupt = true;    
    msg.type = 0x44;
    msg.data = (pu8) "Hello2";
    msg.len = 7;
    Send(demo_tf, &msg);
}


// a made up custom checksum - just to test it's used and works

CKSUM CksumStart(void)
{
    return 0;
}

CKSUM CksumAdd(CKSUM cksum, uint8_t byte)
{
    return cksum ^ byte + 1;
}

CKSUM CksumEnd(CKSUM cksum)
{
    return ~cksum ^ 0xA5;
}
