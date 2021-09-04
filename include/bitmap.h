#pragma once

#include "compiler.h"
#include "macros.h"
#include "types.h"

// Represents a 'bitmap' e.g. a collection of bits.
struct bitmap {
	uint32_t num_bits;
	uint32_t num_words;
	uint64_t data[];
};

// Determines the number of uint64_t words the specified number of bits will
// require.
static inline uint64_t bitmap_calc_words(uint64_t num_bits)
{
	return ALIGN_UP(num_bits, 64) / 64;
}

// Determines the number of bytes a bitmap of `num_bits` bits will contain.
static inline uint64_t bitmap_calc_size(uint64_t num_bits)
{
	return sizeof(struct bitmap) + bitmap_calc_words(num_bits) * 8;
}

// Clears all bits contained within the bitmap.
static inline void bitmap_clear_all(struct bitmap *bitmap)
{
	uint32_t num_words = bitmap->num_words;
	for (uint32_t i = 0; i < num_words; i++) {
		bitmap->data[i] = 0;
	}
}

// Set all bits contained within the bitmap.
static inline void bitmap_set_all(struct bitmap *bitmap)
{
	uint32_t num_words = bitmap->num_words;
	for (uint32_t i = 0; i < num_words; i++) {
		bitmap->data[i] = ~0UL;
	}

	// Clear out-of-range bits.
	uint64_t final_word_mask = (1UL << (bitmap->num_bits % 64)) - 1;
	if (final_word_mask != 0)
		bitmap->data[num_words - 1] &= final_word_mask;
}

// Initialises the specified bitmap to contain `num_bits` bits. Returns the
// bitmap for convenience.
static inline struct bitmap *bitmap_init(struct bitmap *bitmap,
					 uint64_t num_bits)
{
	bitmap->num_bits = num_bits;
	bitmap->num_words = bitmap_calc_words(num_bits);
	bitmap_clear_all(bitmap);

	return bitmap;
}

// Set the specified bit in the bitmap.
static inline void bitmap_set(struct bitmap *bitmap, uint64_t bit)
{
	uint64_t word = bit / 64;
	uint64_t offset = bit % 64;
	uint64_t mask = (1UL << offset);

	bitmap->data[word] |= mask;
}

// Clear the specified bit in the bitmap.
static inline void bitmap_clear(struct bitmap *bitmap, uint64_t bit)
{
	uint64_t word = bit / 64;
	uint64_t offset = bit % 64;
	uint64_t mask = (1UL << offset);

	bitmap->data[word] ^= mask;
}

// Determine if the specified bit is set in the bitmap.
static inline bool bitmap_is_set(struct bitmap *bitmap, uint64_t bit)
{
	uint64_t word = bit / 64;
	uint64_t offset = bit % 64;
	uint64_t mask = (1UL << offset);

	return !!(bitmap->data[word] & mask);
}

// Determine the index of the first set bit, or -1 if no bit set.
static inline int64_t bitmap_first_set(struct bitmap *bitmap)
{
	for (uint32_t i = 0; i < bitmap->num_words; i++) {
		uint64_t word = bitmap->data[i];
		int64_t index = find_first_set_bit(word);
		if (index != -1)
			return i * 64 + index;
	}

	return -1;
}

// Determine the index of the first clear bit, or -1 if all bits set.
static inline int64_t bitmap_first_clear(struct bitmap *bitmap)
{
	int64_t ret = -1;
	for (uint32_t i = 0; i < bitmap->num_words; i++) {
		uint64_t word = bitmap->data[i];
		int64_t index = find_first_clear_bit(word);

		if (index != -1) {
			ret = i * 64 + index;
			break;
		}
	}

	// We keep out-of-range bits clear so we need to check to ensure we
	// don't return an out-of-range index.
	return ret >= bitmap->num_bits ? -1 : ret;
}
