#ifndef TYPE_COERCE_H
#define TYPE_COERCE_H

/**
 * Structs for conversion between types,
 * part of the TinyFrame utilities collection
 * 
 * (c) Ondřej Hruška, 2016-2017. MIT license.
 * 
 * This is a support header file for PayloadParser and PayloadBuilder.
 */

#include <stdint.h>
#include <stddef.h>

union conv8 {
    uint8_t u8;
    int8_t i8;
};

union conv16 {
    uint16_t u16;
    int16_t i16;
};

union conv32 {
    uint32_t u32;
    int32_t i32;
    float f32;
};

#endif // TYPE_COERCE_H
