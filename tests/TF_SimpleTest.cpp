#include "../TinyFrame.hpp"
#include "../TinyFrame_CRC.hpp"

#include <iostream>
#include <string>
#include <cassert>

using TinyFrame_CRC16 = TinyFrame_n::TinyFrame<TinyFrame_n::TF_CKSUM_t::CRC16>;

extern TinyFrame_CRC16 tf_1;
extern TinyFrame_CRC16 tf_2;
namespace TinyFrame_n{

void TF_WriteImpl_1(const uint8_t *buff, uint32_t len)
{
    tf_2.TF_Accept(buff, len);
    printf("TF_WriteImpl1:");
    // send to UART
    for (size_t i = 0; i < len; i++){
        printf("%02x", buff[i]);
    }
    printf("\n");
}

void TF_WriteImpl_2(const uint8_t *buff, uint32_t len)
{
    tf_1.TF_Accept(buff, len);
    printf("TF_WriteImpl2:");
    // send to UART
    for (size_t i = 0; i < len; i++){
        printf("%02x", buff[i]);
    }
    printf("\n");
}

void TF_Error_1(std::string message){
    
}

void TF_Error_2(std::string message){

}

// --------- Mutex callbacks ----------
// Needed only if TF_USE_MUTEX is 1 in the config file.
// DELETE if mutex is not used

/** Claim the TX interface before composing and sending a frame */
bool TF_ClaimTx()
{
    // take mutex
    return true; // we succeeded
}

/** Free the TX interface after composing and sending a frame */
void TF_ReleaseTx()
{
    // release mutex
}

// --------- Custom checksums ---------
// This should be defined here only if a custom checksum type is used.
// DELETE those if you use one of the built-in checksum types

/** Initialize a checksum */
template<>
TF_CKSUM<TF_CKSUM_t::CUSTOM8> TF_CksumStart<TF_CKSUM_t::CUSTOM8>(void)
{
    return 0;
}

/** Update a checksum with a byte */
template<>
TF_CKSUM<TF_CKSUM_t::CUSTOM8> TF_CksumAdd<TF_CKSUM_t::CUSTOM8>(TF_CKSUM<TF_CKSUM_t::CUSTOM8> cksum, uint8_t byte)
{
    return cksum ^ byte;
}

/** Finalize the checksum calculation */
template<>
TF_CKSUM<TF_CKSUM_t::CUSTOM8> TF_CksumEnd<TF_CKSUM_t::CUSTOM8>(TF_CKSUM<TF_CKSUM_t::CUSTOM8> cksum)
{
    return cksum;
}


// --------- Example Type Listener ---------
TF_Result genericListener(TF_Msg *msg){
    printf("Received Listener 123 Message: %s", msg->data);    
    return TF_STAY;
}

TF_Result typeListener123(TF_Msg *msg){
    printf("Received Listener 123 Message: %s", msg->data);    
    return TF_STAY;
}

TF_Result idListener234(TF_Msg *msg){
    printf("Received ID Listener 234 Message: %s", msg->data);    
    return TF_CLOSE;
}
TF_Result idTimeoutListener234(){
    printf("Received ID Listener 234 Message");
    return TF_STAY;
}

} // TinyFrame_n



const TinyFrame_CRC16::TF_RequiredCallbacks callbacks_1 = {
    .TF_WriteImpl = TinyFrame_n::TF_WriteImpl_1,
    .TF_Error = TinyFrame_n::TF_Error_1,
    .TF_ClaimTx =   TinyFrame_n::TF_ClaimTx, 
    .TF_ReleaseTx = TinyFrame_n::TF_ReleaseTx,
};

const TinyFrame_CRC16::TF_RequiredCallbacks callbacks_2 = {
    .TF_WriteImpl = TinyFrame_n::TF_WriteImpl_2,
    .TF_Error = TinyFrame_n::TF_Error_2,
    .TF_ClaimTx =   TinyFrame_n::TF_ClaimTx, 
    .TF_ReleaseTx = TinyFrame_n::TF_ReleaseTx,
};

TinyFrame_n::TinyFrameConfig_t config = {
    .TF_ID_BYTES             = 1U,
    .TF_LEN_BYTES            = 2U,
    .TF_TYPE_BYTES           = 1U,
    .TF_USE_SOF_BYTE         = 1U,
    .TF_SOF_BYTE             = 0xAAU,
    .TF_PARSER_TIMEOUT_TICKS = 10U
};

TinyFrame_CRC16 tf_1(callbacks_1);
TinyFrame_CRC16 tf_2(callbacks_2);

// TinyFrame_CRC16 tf16(callbacks16);
// TinyFrame_CRC32 tf32_2(callbacks32, config);


int main(){

    uint8_t messageData[] = "Hello TinyFrame!";

    TinyFrame_n::TF_Msg msg = {
        .frame_id    = 234,
        .is_response = false,
        .type        = 123,
        .data        = messageData,
        .len         = sizeof(messageData)
    };

    bool successGenericListener = tf_2.TF_AddGenericListener(&TinyFrame_n::genericListener);
    assert(successGenericListener);
    bool successTypeListener = tf_2.TF_AddTypeListener(123, &TinyFrame_n::typeListener123);
    assert(successTypeListener);
    bool successIDListener = tf_2.TF_AddIdListener(&msg, &TinyFrame_n::idListener234, &TinyFrame_n::idTimeoutListener234, 10);
    assert(successIDListener);

    for (size_t i = 0; i < 5; i++){
        tf_2.TF_Tick();
    }


    bool successSend = tf_1.TF_Send(&msg);
    assert(successSend);
}