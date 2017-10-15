//
// Rename to TF_Config.h
//

#ifndef TF_CONFIG_H
#define TF_CONFIG_H

#include <stdint.h>
//#include <esp8266.h> // when using with esphttpd

//----------------------------- FRAME FORMAT ---------------------------------
// The format can be adjusted to fit your particular application needs

// If the connection is reliable, you can disable the SOF byte and checksums.
// That can save up to 9 bytes of overhead.

// ,-----+----+-----+------+------------+- - - -+------------,
// | SOF | ID | LEN | TYPE | HEAD_CKSUM | DATA  | PLD_CKSUM  |
// | 1   | ?  | ?   | ?    | ?          | ...   | ?          | <- size (bytes)
// '-----+----+-----+------+------------+- - - -+------------'

// !!! BOTH SIDES MUST USE THE SAME SETTINGS !!!

// Adjust sizes as desired (1,2,4)
#define TF_ID_BYTES     1
#define TF_LEN_BYTES    2
#define TF_TYPE_BYTES   1

// Checksum type
//#define TF_CKSUM_TYPE TF_CKSUM_NONE
//#define TF_CKSUM_TYPE TF_CKSUM_XOR
#define TF_CKSUM_TYPE TF_CKSUM_CRC16
//#define TF_CKSUM_TYPE TF_CKSUM_CRC32

// Use a SOF byte to mark the start of a frame
#define TF_USE_SOF_BYTE 1
// Value of the SOF byte (if TF_USE_SOF_BYTE == 1)
#define TF_SOF_BYTE     0x01

//----------------------- PLATFORM COMPATIBILITY ----------------------------

// used for timeout tick counters - should be large enough for all used timeouts
typedef uint16_t TF_TICKS;

// used in loops iterating over listeners
typedef uint8_t TF_COUNT;

//----------------------------- PARAMETERS ----------------------------------

// Maximum send / receive payload size (static buffers size)
// Larger payloads will be rejected.
#define TF_MAX_PAYLOAD_RX 1024
#define TF_MAX_PAYLOAD_TX 1024

// --- Listener counts - determine sizes of the static slot tables ---

// Frame ID listeners (wait for response / multi-part message)
#define TF_MAX_ID_LST   10
// Frame Type listeners (wait for frame with a specific first payload byte)
#define TF_MAX_TYPE_LST 10
// Generic listeners (fallback if no other listener catches it)
#define TF_MAX_GEN_LST  5

// Timeout for receiving & parsing a frame
// ticks = number of calls to TF_Tick()
#define TF_PARSER_TIMEOUT_TICKS 10

//------------------------- End of user config ------------------------------

#endif //TF_CONFIG_H
