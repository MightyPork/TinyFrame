//
// TinyFrame configuration file.
//
// Rename to TF_Config.h
//

#ifndef TF_CONFIG_H
#define TF_CONFIG_H

#include <cstdint>

namespace TinyFrame_n{

const struct TinyFrameConfig_t{
//----------------------------- FRAME FORMAT ---------------------------------
// The format can be adjusted to fit your particular application needs

// If the connection is reliable, you can disable the SOF byte and checksums.
// That can save up to 9 bytes of overhead.

// ,-----+-----+-----+------+------------+- - - -+-------------,                
// | SOF | ID  | LEN | TYPE | HEAD_CKSUM | DATA  | DATA_CKSUM  |                
// | 0-1 | 1-4 | 1-4 | 1-4  | 0-4        | ...   | 0-4         | <- size (bytes)
// '-----+-----+-----+------+------------+- - - -+-------------'                

// !!! BOTH PEERS MUST USE THE SAME SETTINGS !!!

// Adjust sizes as desired (1,2,4)
uint8_t TF_ID_BYTES =     1U;
uint8_t TF_LEN_BYTES =    2U;
uint8_t TF_TYPE_BYTES =   1U;

// Use a SOF byte to mark the start of a frame
uint8_t TF_USE_SOF_BYTE = 1U;
// Value of the SOF byte (if TF_USE_SOF_BYTE == 1)
uint8_t TF_SOF_BYTE =     0x01U;

// Timeout for receiving & parsing a frame
// ticks = number of calls to TF_Tick()
uint8_t TF_PARSER_TIMEOUT_TICKS = 10U;

}TF_CONFIG_DEFAULT;
//----------------------- PLATFORM COMPATIBILITY ----------------------------

// used for timeout tick counters - should be large enough for all used timeouts
typedef size_t TF_TICKS;

// used in loops iterating over listeners
typedef size_t TF_COUNT;

//----------------------------- PARAMETERS ----------------------------------

// --- Listener counts - determine sizes of the static slot tables ---

// Frame ID listeners (wait for response / multi-part message)
#define TF_MAX_ID_LST   10
// Frame Type listeners (wait for frame with a specific first payload byte)
#define TF_MAX_TYPE_LST 10
// Generic listeners (fallback if no other listener catches it)
#define TF_MAX_GEN_LST  5

// Whether to use mutex - requires you to implement TF_ClaimTx() and TF_ReleaseTx()
#define TF_USE_MUTEX  1

//------------------------- End of user config ------------------------------
} // TinyFrame_n
#endif //TF_CONFIG_H
