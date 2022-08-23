#include <cstdio>
#include <cstring>
#include "../../TinyFrame.hpp"
#include "../utils.hpp"

using namespace TinyFrame_n;

using TinyFrame_Default=TinyFrame<>;

extern TinyFrame_Default demo_tf;

bool do_corrupt = false;

/**
 * This function should be defined in the application code.
 * It implements the lowest layer - sending bytes to UART (or other)
 */
void WriteImpl(const uint8_t *buff, uint32_t len)
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
    demo_tf.Accept(xbuff, len);
}

void Error(std::string message){
    printf("%s", message);
}


/** An example listener function */
Result myListener(Msg *msg)
{
    dumpFrameInfo(msg);
    return Result::STAY;
}

Result testIdListener(Msg *msg)
{
    printf("OK - ID Listener triggered for msg!\n");
    dumpFrameInfo(msg);
    return Result::CLOSE;
}

TinyFrame_Default demo_tf(TinyFrame_Default::RequiredCallbacks{
    .WriteImpl = WriteImpl,
    .Error = Error
}, Peer::MASTER);

int main(void)
{
    Msg msg;
    const char *longstr = "Lorem ipsum dolor sit amet.";

    // Set up the TinyFrame library
    demo_tf.AddGenericListener(myListener);

    printf("------ Simulate sending a message --------\n");

    demo_tf.ClearMsg(&msg);
    msg.type = 0x22;
    msg.data = (pu8) "Hello TinyFrame";
    msg.len = 16;
    demo_tf.Send(&msg);

    msg.type = 0x33;
    msg.data = (pu8) longstr;
    msg.len = (LEN) (strlen(longstr) + 1); // add the null byte
    demo_tf.Send(&msg);

    msg.type = 0x44;
    msg.data = (pu8) "Hello2";
    msg.len = 7;
    demo_tf.Send(&msg);

    msg.len = 0;
    msg.type = 0x77;
    demo_tf.Query(&msg, testIdListener, nullptr, 0);
    
    printf("This should fail:\n");
    
    // test checksums are tested
    do_corrupt = true;    
    msg.type = 0x44;
    msg.data = (pu8) "Hello2";
    msg.len = 7;
    demo_tf.Send(&msg);
    return 0;
}
