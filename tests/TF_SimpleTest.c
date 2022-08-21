#include "TinyFrame.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * This is an example of integrating TinyFrame into the application.
 * 
 * TF_WriteImpl() is required, the mutex functions are weak and can
 * be removed if not used. They are called from all TF_Send/Respond functions.
 * 
 * Also remember to periodically call TF_Tick() if you wish to use the 
 * listener timeout feature.
 */


extern TinyFrame tf8_1;
extern TinyFrame tf8_2;


void TF_WriteImpl(TinyFrame* tf, const uint8_t *buff, uint32_t len)
{

    TF_Accept(&tf8_2, buff, len);
    printf("TF_WriteImpl1:");
    // send to UART
    for (size_t i = 0; i < len; i++){
        printf("%02x", buff[i]);
    }
    printf("\n");
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
TF_CKSUM TF_CksumStart(void)
{
    return 0;
}

/** Update a checksum with a byte */
TF_CKSUM TF_CksumAdd(TF_CKSUM cksum, uint8_t byte)
{
    return cksum ^ byte;
}

/** Finalize the checksum calculation */
TF_CKSUM TF_CksumEnd(TF_CKSUM cksum)
{
    return cksum;
}

TF_Result typeListener123(TinyFrame *tf, TF_Msg *msg){
    printf("Received Listener 123 Message: %s", msg->data);    
    return TF_STAY;
}

TinyFrame tf8_1;
TinyFrame tf8_2;

int main(){

    uint8_t messageData[] = "Hello TinyFrame!";

    TF_Msg msg = {
        .frame_id    = 0,
        .is_response = false,
        .type        = 123,
        .data        = messageData,
        .len         = sizeof(messageData)
    };

    TF_AddTypeListener(&tf8_2, 123, &typeListener123);

    TF_Send(&tf8_1, &msg);
}