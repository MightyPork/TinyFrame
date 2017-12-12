#include <string.h>
#include "payload_builder.h"

#define pb_check_capacity(pb, needed) \
    if ((pb)->current + (needed) > (pb)->end) { \
        if ((pb)->full_handler == NULL || !(pb)->full_handler(pb, needed)) (pb)->ok = 0; \
    }

/** Write from a buffer */
bool pb_buf(PayloadBuilder *pb, const uint8_t *buf, uint32_t len)
{
    pb_check_capacity(pb, len);
    if (!pb->ok) return false;

    memcpy(pb->current, buf, len);
    pb->current += len;
    return true;
}

/** Write s zero terminated string */
bool pb_string(PayloadBuilder *pb, const char *str)
{
    uint32_t len = (uint32_t) strlen(str);
    pb_check_capacity(pb, len+1);
    if (!pb->ok) return false;

    memcpy(pb->current, str, len+1);
    pb->current += len+1;
    return true;
}

/** Write uint8_t to the buffer */
bool pb_u8(PayloadBuilder *pb, uint8_t byte)
{
    pb_check_capacity(pb, 1);
    if (!pb->ok) return false;

    *pb->current++ = byte;
    return true;
}

/** Write uint16_t to the buffer. */
bool pb_u16(PayloadBuilder *pb, uint16_t word)
{
    pb_check_capacity(pb, 2);
    if (!pb->ok) return false;

    if (pb->bigendian) {
        *pb->current++ = (uint8_t) ((word >> 8) & 0xFF);
        *pb->current++ = (uint8_t) (word & 0xFF);
    } else {
        *pb->current++ = (uint8_t) (word & 0xFF);
        *pb->current++ = (uint8_t) ((word >> 8) & 0xFF);
    }
    return true;
}

/** Write uint32_t to the buffer. */
bool pb_u32(PayloadBuilder *pb, uint32_t word)
{
    pb_check_capacity(pb, 4);
    if (!pb->ok) return false;

    if (pb->bigendian) {
        *pb->current++ = (uint8_t) ((word >> 24) & 0xFF);
        *pb->current++ = (uint8_t) ((word >> 16) & 0xFF);
        *pb->current++ = (uint8_t) ((word >> 8) & 0xFF);
        *pb->current++ = (uint8_t) (word & 0xFF);
    } else {
        *pb->current++ = (uint8_t) (word & 0xFF);
        *pb->current++ = (uint8_t) ((word >> 8) & 0xFF);
        *pb->current++ = (uint8_t) ((word >> 16) & 0xFF);
        *pb->current++ = (uint8_t) ((word >> 24) & 0xFF);
    }
    return true;
}

/** Write int8_t to the buffer. */
bool pb_i8(PayloadBuilder *pb, int8_t byte)
{
    return pb_u8(pb, ((union conv8){.i8 = byte}).u8);
}

/** Write int16_t to the buffer. */
bool pb_i16(PayloadBuilder *pb, int16_t word)
{
    return pb_u16(pb, ((union conv16){.i16 = word}).u16);
}

/** Write int32_t to the buffer. */
bool pb_i32(PayloadBuilder *pb, int32_t word)
{
    return pb_u32(pb, ((union conv32){.i32 = word}).u32);
}

/** Write 4-byte float to the buffer. */
bool pb_float(PayloadBuilder *pb, float f)
{
    return pb_u32(pb, ((union conv32){.f32 = f}).u32);
}
