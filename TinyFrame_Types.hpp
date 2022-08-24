#ifndef TinyFrame_TypesHPP
#define TinyFrame_TypesHPP

#include <cstdint>  // uint8_t etc
#include <cstdlib> // size_t

namespace TinyFrame_n{

// Checksum type (0 = none, 8 = ~XOR, 16 = CRC16 0x8005, 32 = CRC32)
enum class CKSUM_t{
    NONE     = 0,  // no checksums
    CUSTOM8  = 1,  // Custom 8-bit checksum
    CUSTOM16 = 2,  // Custom 16-bit checksum
    CUSTOM32 = 3,  // Custom 32-bit checksum
    XOR      = 8,  // inverted xor of all payload bytes
    CRC8     = 9,  // Dallas/Maxim CRC8 (1-wire)
    CRC16    = 16, // CRC16 with the polynomial 0x8005 (x^16 + x^15 + x^2 + 1)
    CRC32    = 32, // CRC32 with the polynomial 0xedb88320
};

//region Resolve data types

typedef size_t LEN; // LEN_BYTES
typedef size_t TYPE; // TYPE_BYTES
typedef size_t ID; // ID_BYTES

// TODO: runtime checks
// #if ID_BYTES != 1 and ID_BYTES != 2 and ID_BYTES != 4
//     #error Bad value of ID_BYTES, must be 1, 2 or 4
// #endif

typedef size_t TICKS; // used for timeout tick counters - should be large enough for all used timeouts
typedef size_t COUNT; // used in loops iterating over listeners

#define _FN

template <CKSUM_t CKSUM_TYPE>
struct CKSUM_TYPE_MAP;

template <> struct CKSUM_TYPE_MAP<CKSUM_t::NONE> {using type = uint8_t;};
template <> struct CKSUM_TYPE_MAP<CKSUM_t::XOR> {using type = uint8_t;};
template <> struct CKSUM_TYPE_MAP<CKSUM_t::CUSTOM8> {using type = uint8_t;};
template <> struct CKSUM_TYPE_MAP<CKSUM_t::CRC8> {using type = uint8_t;};

template <> struct CKSUM_TYPE_MAP<CKSUM_t::CRC16> {using type = uint16_t;};
template <> struct CKSUM_TYPE_MAP<CKSUM_t::CUSTOM16> {using type = uint16_t;};

template <> struct CKSUM_TYPE_MAP<CKSUM_t::CRC32> {using type = uint32_t;};
template <> struct CKSUM_TYPE_MAP<CKSUM_t::CUSTOM32> {using type = uint32_t;};


template<CKSUM_t CKSUM_TYPE>
using CKSUM = typename CKSUM_TYPE_MAP<CKSUM_TYPE>::type;

//endregion

//---------------------------------------------------------------------------

/** Peer bit enum (used for init) */
enum class Peer{
    SLAVE = 0,
    MASTER = 1,
};


/** Response from listeners */
enum class Result{
    NEXT = 0,   //!< Not handled, let other listeners handle it
    STAY = 1,   //!< Handled, stay
    RENEW = 2,  //!< Handled, stay, renew - useful only with listener timeout
    CLOSE = 3,  //!< Handled, remove self
};


// ---------------------------------- INTERNAL ----------------------------------
// This is publicly visible only to allow static init.

enum class State_ {
    TFState_SOF = 0,      //!< Wait for SOF
    TFState_LEN,          //!< Wait for Number Of Bytes
    TFState_HEAD_CKSUM,   //!< Wait for header Checksum
    TFState_ID,           //!< Wait for ID
    TFState_TYPE,         //!< Wait for message type
    TFState_DATA,         //!< Receive payload
    TFState_DATA_CKSUM    //!< Wait for Checksum
};

constexpr struct TinyFrameConfig_t{
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
uint8_t ID_BYTES =     1U;
uint8_t LEN_BYTES =    2U;
uint8_t TYPE_BYTES =   1U;

// Use a SOF byte to mark the start of a frame
uint8_t USE_SOF_BYTE = 1U;
// Value of the SOF byte (if USE_SOF_BYTE == 1)
uint8_t SOF_BYTE =     0x01U;

// Timeout for receiving & parsing a frame
// ticks = number of calls to Tick()
uint8_t PARSER_TIMEOUT_TICKS = 10U;

}CONFIG_DEFAULT;

/** Data structure for sending / receiving messages */
struct Msg{
    ID frame_id;       //!< message ID
    bool is_response;     //!< internal flag, set when using the Respond function. frame_id is then kept unchanged.
    TYPE type;         //!< received or sent message type

    /**
     * Buffer of received data, or data to send.
     *
     * - If (data == nullptr) in an ID listener, that means the listener timed out and
     *   the user should free any userdata and take other appropriate actions.
     *
     * - If (data == nullptr) and length is not zero when sending a frame, that starts a multi-part frame.
     *   This call then must be followed by sending the payload and closing the frame.
     */
    const uint8_t *data;
    LEN len; //!< length of the payload

    /**
     * Custom user data for the ID listener.
     *
     * This data will be stored in the listener slot and passed to the ID callback
     * via those same fields on the received message.
     */
    void *userdata;
    void *userdata2;

};

} // TinyFrame_n

#endif // TinyFrame_TypesHPP