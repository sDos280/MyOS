// bitmap_util.c

#include "utils/bitmap_util.h"


uint8_t bitmap_get(const uint8_t *bitmap, uint32_t idx)
{
    return (bitmap[idx / 8] >> (idx % 8)) & 1;
}

void bitmap_set(uint8_t *bitmap, uint32_t idx)
{
    bitmap[idx / 8] |= (1 << (idx % 8));
}

void bitmap_clear(uint8_t *bitmap, uint32_t idx)
{
    bitmap[idx / 8] &= ~(1 << (idx % 8));
}

uint8_t bitmap_find_first_clear(const uint8_t *bitmap, uint32_t num_bits,
                                 uint32_t *idx_out)
{
    uint32_t num_bytes = (num_bits + 7) / 8;

    for (uint32_t byte = 0; byte < num_bytes; byte++) {

        /* Fast skip — all 8 bits used */
        if (bitmap[byte] == 0xFF)
            continue;

        for (uint32_t bit = 0; bit < 8; bit++) {
            uint32_t idx = (byte * 8) + bit;

            /* Don't walk past the declared size */
            if (idx >= num_bits)
                return 0;

            if (!((bitmap[byte] >> bit) & 1)) {
                *idx_out = idx;
                return 1;
            }
        }
    }

    return 0;
}

uint32_t bitmap_count_clear(const uint8_t *bitmap, uint32_t num_bits)
{
    uint32_t count     = 0;
    uint32_t num_bytes = (num_bits + 7) / 8;

    for (uint32_t byte = 0; byte < num_bytes; byte++) {

        /* Fast path — all 8 bits free */
        if (bitmap[byte] == 0x00) {
            uint32_t remaining = num_bits - (byte * 8);
            count += (remaining >= 8) ? 8 : remaining;
            continue;
        }

        /* Fast path — all 8 bits used */
        if (bitmap[byte] == 0xFF)
            continue;

        for (uint32_t bit = 0; bit < 8; bit++) {
            uint32_t idx = (byte * 8) + bit;

            if (idx >= num_bits)
                break;

            if (!((bitmap[byte] >> bit) & 1))
                count++;
        }
    }

    return count;
}