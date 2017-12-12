#ifndef PAYLOAD_BUILDER_H
#define PAYLOAD_BUILDER_H

/**
 * PayloadBuilder, part of the TinyFrame utilities collection
 * 
 * (c) Ondřej Hruška, 2014-2017. MIT license.
 * 
 * The builder supports big and little endian which is selected when 
 * initializing it or by accessing the bigendian struct field.
 * 
 * This module helps you with building payloads (not only for TinyFrame)
 *
 * The builder performs bounds checking and calls the provided handler when 
 * the requested write wouldn't fit. Use the handler to realloc / flush the buffer
 * or report an error.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "type_coerce.h"

typedef struct PayloadBuilder_ PayloadBuilder;

/**
 * Full buffer handler. 
 * 
 * 'needed' more bytes should be written but the end of the buffer was reached.
 * 
 * Return true if the problem was solved (e.g. buffer was flushed and the 
 * 'current' pointer moved to the beginning).
 * 
 * If false is returned, the 'ok' flag on the struct is set to false
 * and all following writes are discarded.
 */
typedef bool (*pb_full_handler)(PayloadBuilder *pb, uint32_t needed);

struct PayloadBuilder_ {
    uint8_t *start;   //!< Pointer to the beginning of the buffer
    uint8_t *current; //!< Pointer to the next byte to be read
    uint8_t *end;     //!< Pointer to the end of the buffer (start + length)
    pb_full_handler full_handler; //!< Callback for buffer overrun
    bool bigendian;   //!< Flag to use big-endian parsing
    bool ok;          //!< Indicates that all reads were successful
};

// --- initializer helper macros ---

/** Start the builder. */
#define pb_start_e(buf, capacity, bigendian, full_handler) ((PayloadBuilder){buf, buf, (buf)+(capacity), full_handler, bigendian, 1})

/** Start the builder in big-endian mode */
#define pb_start_be(buf, capacity, full_handler) pb_start_e(buf, capacity, 1, full_handler)

/** Start the builder in little-endian mode */
#define pb_start_le(buf, capacity, full_handler) pb_start_e(buf, capacity, 0, full_handler)

/** Start the parser in little-endian mode (default) */
#define pb_start(buf, capacity, full_handler) pb_start_le(buf, capacity, full_handler)

// --- utilities ---

/** Get already used bytes count */
#define pb_length(pb) ((pb)->current - (pb)->start)

/** Reset the current pointer to start */
#define pb_rewind(pb) do { pb->current = pb->start; } while (0)


/** Write from a buffer */
bool pb_buf(PayloadBuilder *pb, const uint8_t *buf, uint32_t len);

/** Write a zero terminated string */
bool pb_string(PayloadBuilder *pb, const char *str);

/** Write uint8_t to the buffer */
bool pb_u8(PayloadBuilder *pb, uint8_t byte);

/** Write boolean to the buffer. */
static inline bool pb_bool(PayloadBuilder *pb, bool b)
{
    return pb_u8(pb, (uint8_t) b);
}

/** Write uint16_t to the buffer. */
bool pb_u16(PayloadBuilder *pb, uint16_t word);

/** Write uint32_t to the buffer. */
bool pb_u32(PayloadBuilder *pb, uint32_t word);

/** Write int8_t to the buffer. */
bool pb_i8(PayloadBuilder *pb, int8_t byte);

/** Write char (int8_t) to the buffer. */
static inline bool pb_char(PayloadBuilder *pb, char c)
{
    return pb_i8(pb, c);
}

/** Write int16_t to the buffer. */
bool pb_i16(PayloadBuilder *pb, int16_t word);

/** Write int32_t to the buffer. */
bool pb_i32(PayloadBuilder *pb, int32_t word);

/** Write 4-byte float to the buffer. */
bool pb_float(PayloadBuilder *pb, float f);

#endif // PAYLOAD_BUILDER_H
