#include "TinyFrame.h"

/**
 * This is an example of integrating TinyFrame into the application.
 * 
 * TF_WriteImpl() is required, the mutex functions are weak and can
 * be removed if not used. They are called from all TF_Send/Respond functions.
 * 
 * Also remember to periodically call TF_Tick() if you wish to use the 
 * listener timeout feature.
 */

void TF_WriteImpl(const uint8_t *buff, size_t len)
{
    // send to UART
}

/** Claim the TX interface before composing and sending a frame */
void TF_ClaimTx(void)
{
    // take mutex
}

/** Free the TX interface after composing and sending a frame */
void TF_ReleaseTx(void)
{
    // release mutex
}
