// bitmap_util.h

#ifndef BITMAP_UTIL_H
#define BITMAP_UTIL_H

#include <stdint.h>

/**
 * bitmap_get - Test whether a bit is set.
 *
 * @bitmap  Pointer to the bitmap byte array.
 * @idx     Bit index (0-based).
 * @return  1 if the bit is set, 0 if clear.
 */
uint8_t bitmap_get(const uint8_t *bitmap, uint32_t idx);

/**
 * bitmap_set - Set a bit to 1 (mark used).
 *
 * @bitmap  Pointer to the bitmap byte array.
 * @idx     Bit index (0-based).
 */
void bitmap_set(uint8_t *bitmap, uint32_t idx);

/**
 * bitmap_clear - Clear a bit to 0 (mark free).
 *
 * @bitmap  Pointer to the bitmap byte array.
 * @idx     Bit index (0-based).
 */
void bitmap_clear(uint8_t *bitmap, uint32_t idx);

/**
 * bitmap_find_first_clear - Find the first clear (free) bit in a bitmap.
 *
 * @bitmap      Pointer to the bitmap byte array.
 * @num_bits    Total number of bits to search.
 * @idx_out     Out: index of the first clear bit found.
 * @return      1 if a clear bit was found, 0 if all bits are set.
 */
uint8_t bitmap_find_first_clear(const uint8_t *bitmap, uint32_t num_bits,
                                 uint32_t *idx_out);

/**
 * bitmap_count_clear - Count the number of clear (free) bits in a bitmap.
 *
 * @bitmap      Pointer to the bitmap byte array.
 * @num_bits    Total number of bits to count.
 * @return      Number of clear bits.
 */
uint32_t bitmap_count_clear(const uint8_t *bitmap, uint32_t num_bits);

#endif // BITMAP_UTIL_H