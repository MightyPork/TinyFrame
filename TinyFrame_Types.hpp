#ifndef TinyFrame_TypesHPP
#define TinyFrame_TypesHPP

#include <stdint.h>  // for uint8_t etc
#include <stdbool.h> // for bool
#include <stddef.h>  // for NULL

typedef uint16_t TF_TICKS;
typedef uint8_t TF_COUNT;

/** Peer bit enum (used for init) */
typedef enum {
    TF_SLAVE = 0,
    TF_MASTER = 1,
} TF_Peer;


/** Response from listeners */
typedef enum {
    TF_NEXT = 0,   //!< Not handled, let other listeners handle it
    TF_STAY = 1,   //!< Handled, stay
    TF_RENEW = 2,  //!< Handled, stay, renew - useful only with listener timeout
    TF_CLOSE = 3,  //!< Handled, remove self
} TF_Result;


// ---------------------------------- INTERNAL ----------------------------------
// This is publicly visible only to allow static init.

enum TF_State_ {
    TFState_SOF = 0,      //!< Wait for SOF
    TFState_LEN,          //!< Wait for Number Of Bytes
    TFState_HEAD_CKSUM,   //!< Wait for header Checksum
    TFState_ID,           //!< Wait for ID
    TFState_TYPE,         //!< Wait for message type
    TFState_DATA,         //!< Receive payload
    TFState_DATA_CKSUM    //!< Wait for Checksum
};

#endif // TinyFrame_TypesHPP