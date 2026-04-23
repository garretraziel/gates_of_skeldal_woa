#ifndef UNALIGNED_H
#define UNALIGNED_H

#include <string.h>
#include <stdint.h>

/* Portable unaligned memory access helpers.
 * ARM64 requires aligned access for multi-byte loads/stores.
 * These helpers use memcpy which the compiler optimizes into
 * appropriate unaligned load/store instructions. */

static inline uint16_t read_u16_unaligned(const void *ptr) {
    uint16_t val;
    memcpy(&val, ptr, sizeof(val));
    return val;
}

static inline int16_t read_i16_unaligned(const void *ptr) {
    int16_t val;
    memcpy(&val, ptr, sizeof(val));
    return val;
}

static inline uint32_t read_u32_unaligned(const void *ptr) {
    uint32_t val;
    memcpy(&val, ptr, sizeof(val));
    return val;
}

static inline int32_t read_i32_unaligned(const void *ptr) {
    int32_t val;
    memcpy(&val, ptr, sizeof(val));
    return val;
}

static inline void write_u16_unaligned(void *ptr, uint16_t val) {
    memcpy(ptr, &val, sizeof(val));
}

static inline void write_i16_unaligned(void *ptr, int16_t val) {
    memcpy(ptr, &val, sizeof(val));
}

static inline void write_u32_unaligned(void *ptr, uint32_t val) {
    memcpy(ptr, &val, sizeof(val));
}

static inline void write_i32_unaligned(void *ptr, int32_t val) {
    memcpy(ptr, &val, sizeof(val));
}

#endif /* UNALIGNED_H */
