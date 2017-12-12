#ifndef PAYLOAD_PARSER_H
#define PAYLOAD_PARSER_H

/**
 * PayloadParser, part of the TinyFrame utilities collection
 * 
 * (c) Ondřej Hruška, 2016-2017. MIT license.
 * 
 * This module helps you with parsing payloads (not only from TinyFrame).
 * 
 * The parser supports big and little-endian which is selected when 
 * initializing it or by accessing the bigendian struct field.
 *
 * The parser performs bounds checking and calls the provided handler when 
 * the requested read doesn't have enough data. Use the callback to take
 * appropriate action, e.g. report an error.
 * 
 * If the handler function is not defined, the pb->ok flag is set to false
 * (use this to check for success), and further reads won't have any effect 
 * and always result in 0 or empty array.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "type_coerce.h"

typedef struct PayloadParser_ PayloadParser;

/**
 * Empty buffer handler. 
 * 
 * 'needed' more bytes should be read but the end was reached.
 * 
 * Return true if the problem was solved (e.g. new data loaded into 
 * the buffer and the 'current' pointer moved to the beginning).
 *  
 * If false is returned, the 'ok' flag on the struct is set to false
 * and all following reads will fail / return 0.
 */
typedef bool (*pp_empty_handler)(PayloadParser *pp, uint32_t needed);

struct PayloadParser_ {
    uint8_t *start;   //!< Pointer to the beginning of the buffer
    uint8_t *current; //!< Pointer to the next byte to be read
    uint8_t *end;     //!< Pointer to the end of the buffer (start + length)
    pp_empty_handler empty_handler; //!< Callback for buffer underrun
    bool bigendian;   //!< Flag to use big-endian parsing
    bool ok;          //!< Indicates that all reads were successful
};

// --- initializer helper macros ---

/** Start the parser. */
#define pp_start_e(buf, length, bigendian, empty_handler) ((PayloadParser){buf, buf, (buf)+(length), empty_handler, bigendian, 1})

/** Start the parser in big-endian mode */
#define pp_start_be(buf, length, empty_handler) pp_start_e(buf, length, 1, empty_handler)

/** Start the parser in little-endian mode */
#define pp_start_le(buf, length, empty_handler) pp_start_e(buf, length, 0, empty_handler)

/** Start the parser in little-endian mode (default) */
#define pp_start(buf, length, empty_handler) pp_start_le(buf, length, empty_handler)

// --- utilities ---

/** Get remaining length */
#define pp_length(pp) ((pp)->end - (pp)->current)

/** Reset the current pointer to start */
#define pp_rewind(pp) do { pp->current = pp->start; } while (0)


/**
 * @brief Get the remainder of the buffer.
 *
 * Returns NULL and sets 'length' to 0 if there are no bytes left.
 *
 * @param pp
 * @param length : here the buffer length will be stored. NULL to do not store.
 * @return the remaining portion of the input buffer
 */
const uint8_t *pp_tail(PayloadParser *pp, uint32_t *length);

/** Read uint8_t from the payload. */
uint8_t pp_u8(PayloadParser *pp);

/** Read bool from the payload. */
static inline int8_t pp_bool(PayloadParser *pp)
{
    return pp_u8(pp) != 0;
}

/** Skip bytes */
void pp_skip(PayloadParser *pp, uint32_t num);

/** Read uint16_t from the payload. */
uint16_t pp_u16(PayloadParser *pp);

/** Read uint32_t from the payload. */
uint32_t pp_u32(PayloadParser *pp);

/** Read int8_t from the payload. */
int8_t pp_i8(PayloadParser *pp);

/** Read char (int8_t) from the payload. */
static inline int8_t pp_char(PayloadParser *pp)
{
    return pp_i8(pp);
}

/** Read int16_t from the payload. */
int16_t pp_i16(PayloadParser *pp);

/** Read int32_t from the payload. */
int32_t pp_i32(PayloadParser *pp);

/** Read 4-byte float from the payload. */
float pp_float(PayloadParser *pp);

/**
 * Parse a zero-terminated string
 *
 * @param pp - parser
 * @param buffer - target buffer
 * @param maxlen - buffer size
 * @return actual number of bytes, excluding terminator
 */
uint32_t pp_string(PayloadParser *pp, char *buffer, uint32_t maxlen);

/**
 * Parse a buffer
 *
 * @param pp - parser
 * @param buffer - target buffer
 * @param maxlen - buffer size
 * @return actual number of bytes, excluding terminator
 */
uint32_t pp_buf(PayloadParser *pp, uint8_t *buffer, uint32_t maxlen);


#endif // PAYLOAD_PARSER_H
