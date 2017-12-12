#include "payload_parser.h"

#define pp_check_capacity(pp, needed) \
    if ((pp)->current + (needed) > (pp)->end) { \
        if ((pp)->empty_handler == NULL || !(pp)->empty_handler(pp, needed)) {(pp)->ok = 0;} ; \
    }

void pp_skip(PayloadParser *pp, uint32_t num)
{
    pp->current += num;
}

uint8_t pp_u8(PayloadParser *pp)
{
    pp_check_capacity(pp, 1);
    if (!pp->ok) return 0;

    return *pp->current++;
}

uint16_t pp_u16(PayloadParser *pp)
{
    pp_check_capacity(pp, 2);
    if (!pp->ok) return 0;

    uint16_t x = 0;

    if (pp->bigendian) {
        x |= *pp->current++ << 8;
        x |= *pp->current++;
    } else {
        x |= *pp->current++;
        x |= *pp->current++ << 8;
    }
    return x;
}

uint32_t pp_u32(PayloadParser *pp)
{
    pp_check_capacity(pp, 4);
    if (!pp->ok) return 0;

    uint32_t x = 0;

    if (pp->bigendian) {
        x |= (uint32_t) (*pp->current++ << 24);
        x |= (uint32_t) (*pp->current++ << 16);
        x |= (uint32_t) (*pp->current++ << 8);
        x |= *pp->current++;
    } else {
        x |= *pp->current++;
        x |= (uint32_t) (*pp->current++ << 8);
        x |= (uint32_t) (*pp->current++ << 16);
        x |= (uint32_t) (*pp->current++ << 24);
    }
    return x;
}

const uint8_t *pp_tail(PayloadParser *pp, uint32_t *length)
{
    int32_t len = (int) (pp->end - pp->current);
    if (!pp->ok || len <= 0) {
        if (length != NULL) *length = 0;
        return NULL;
    }

    if (length != NULL) {
        *length = (uint32_t) len;
    }

    return pp->current;
}

/** Read int8_t from the payload. */
int8_t pp_i8(PayloadParser *pp)
{
    return ((union conv8) {.u8 = pp_u8(pp)}).i8;
}

/** Read int16_t from the payload. */
int16_t pp_i16(PayloadParser *pp)
{
    return ((union conv16) {.u16 = pp_u16(pp)}).i16;
}

/** Read int32_t from the payload. */
int32_t pp_i32(PayloadParser *pp)
{
    return ((union conv32) {.u32 = pp_u32(pp)}).i32;
}

/** Read 4-byte float from the payload. */
float pp_float(PayloadParser *pp)
{
    return ((union conv32) {.u32 = pp_u32(pp)}).f32;
}

/** Read a zstring */
uint32_t pp_string(PayloadParser *pp, char *buffer, uint32_t maxlen)
{
    pp_check_capacity(pp, 1);
    uint32_t len = 0;
    while (len < maxlen-1 && pp->current != pp->end) {
        char c = *buffer++ = *pp->current++;
        if (c == 0) break;
        len++;
    }
    *buffer = 0;
    return len;
}

/** Read a buffer */
uint32_t pp_buf(PayloadParser *pp, uint8_t *buffer, uint32_t maxlen)
{
    uint32_t len = 0;
    while (len < maxlen && pp->current != pp->end) {
        *buffer++ = *pp->current++;
        len++;
    }
    return len;
}
