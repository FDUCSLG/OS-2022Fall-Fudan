#pragma once

#include <common/defines.h>

// bitmap is a compact representation of boolean array.
// consecutive 64 bits are stored in one u64 (BitmapCell).
typedef u64 BitmapCell;

#define BITMAP_BITS_PER_CELL (sizeof(BitmapCell) * 8)
#define BITMAP_TO_NUM_CELLS(size) (((size) + BITMAP_BITS_PER_CELL - 1) / BITMAP_BITS_PER_CELL)

// calculate cell index `idx` and in-cell `offset` from `index`.
#define BITMAP_PARSE_INDEX(index, idx, offset) \
    do { \
        idx = index / BITMAP_BITS_PER_CELL; \
        offset = index % BITMAP_BITS_PER_CELL; \
    } while (false)

// declare a new bitmap with `size` bits.
#define Bitmap(name, size) BitmapCell name[BITMAP_TO_NUM_CELLS(size)]

// initialize a bitmap with `size` bits. All bits are cleared.
void init_bitmap(BitmapCell* bitmap, usize size);

// get the bit at `index`.
static INLINE bool bitmap_get(BitmapCell* bitmap, usize index) {
    usize idx, offset;
    BITMAP_PARSE_INDEX(index, idx, offset);
    return (bitmap[idx] >> offset) & 1;
}

// set the bit at `index` to 1.
static INLINE void bitmap_set(BitmapCell* bitmap, usize index) {
    usize idx, offset;
    BITMAP_PARSE_INDEX(index, idx, offset);
    bitmap[idx] |= BIT(offset);
}

// set the bit at `index` to 0.
static INLINE void bitmap_clear(BitmapCell* bitmap, usize index) {
    usize idx, offset;
    BITMAP_PARSE_INDEX(index, idx, offset);
    bitmap[idx] &= ~BIT(offset);
}
