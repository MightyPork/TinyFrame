#include <cstdio>
#include <cstring>
#include "../../TinyFrame.hpp"
#include "../utils.hpp"

using namespace TinyFrame_n;
using TinyFrame_Demo=TinyFrameDefault;

TinyFrame_Demo *demo_tf;

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
    demo_tf->Accept(xbuff, len);
}

void ErrorCallback(std::string message){
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

int main(void)
{
    Msg msg;
    const char *longstr = "Lorem ipsum dolor sit amet.";

    // Set up the TinyFrame library
    demo_tf = new TinyFrame_Demo({.WriteImpl=&WriteImpl, .Error=&ErrorCallback}, Peer::MASTER); // 1 = master, 0 = slave
    demo_tf->AddGenericListener(myListener);

    printf("------ Simulate sending a message --------\n");

    demo_tf->ClearMsg(&msg);
    msg.type = 0x22;
    msg.data = (pu8) "Hello TinyFrame";
    msg.len = 16;
    demo_tf->Send(&msg);
    
    printf("This should fail:\n");
    
    // test checksums are tested
    do_corrupt = true;    
    msg.type = 0x44;
    msg.data = (pu8) "Hello2";
    msg.len = 7;
    demo_tf->Send(&msg);
}


// a made up custom checksum - just to test it's used and works
template<>
uint8_t TinyFrame_n::CksumStart<CKSUM_t::CUSTOM8>(void)
{
    return 0;
}

template<>
uint8_t TinyFrame_n::CksumAdd<CKSUM_t::CUSTOM8>(uint8_t cksum, uint8_t byte)
{
    return cksum ^ byte + 1;
}

template<>
uint8_t TinyFrame_n::CksumEnd<CKSUM_t::CUSTOM8>(uint8_t cksum)
{
    return ~cksum ^ 0xA5;
}
