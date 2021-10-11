#include "test_user.h"

#include "bitmap.h"
#undef static_assert // zeptux breaks static_assert in c++ :)

#include <sstream>
#include <string>

std::string test_bitmap()
{
	assert(bitmap_calc_size(1) == sizeof(struct bitmap) + 8,
	       "bitmap of size 1 != 16 bytes");
	assert(bitmap_calc_size(63) == sizeof(struct bitmap) + 8,
	       "bitmap of size 63 != 16 bytes");
	assert(bitmap_calc_size(64) == sizeof(struct bitmap) + 8,
	       "bitmap of size 64 != 16 bytes");
	assert(bitmap_calc_size(65) == sizeof(struct bitmap) + 16,
	       "bitmap of size 65 != 24 bytes");
	assert(bitmap_calc_size(150) == sizeof(struct bitmap) + 24,
	       "bitmap of size 150 != 32 bytes");

	// Now we know we can calculate the bitmap's size correct, experiment
	// with a bitmap of 150 bits.
	uint8_t *buf = new uint8_t[bitmap_calc_size(150)];
	struct bitmap *bitmap = bitmap_init((struct bitmap *)buf, 150);
	assert(bitmap->num_bits == 150, "bitmap->num_bits != 150");
	assert(bitmap->num_words == 3, "bitmap->num_words != 3");

	// Ensure all bitmap data is clear.
	const auto check_clear = [&]() -> std::string {
		for (int i = 0; i < 3; i++) {
			if (bitmap->data[i] != 0) {
				std::ostringstream oss;
				oss << "bitmap->data[" << i << "] != 0";

				return oss.str();
			}
		}
		return "";
	};

	std::string check = check_clear();
	assert(check.empty(), check);

	// Check setting, clearing each bit.
	for (int i = 0; i < 150; i++) {
		if (bitmap_is_set(bitmap, i)) {
			std::ostringstream oss;
			oss << "bit " << i << " already set?";
			assert(false, oss.str());
		}
		bitmap_set(bitmap, i);
		if (!bitmap_is_set(bitmap, i)) {
			std::ostringstream oss;
			oss << "bit " << i << " not set?";
			assert(false, oss.str());
		}
		bitmap_clear(bitmap, i);
		if (bitmap_is_set(bitmap, i)) {
			std::ostringstream oss;
			oss << "bit " << i << " not cleared?";
			assert(false, oss.str());
		}
	}

	// Set a few bits and check that we can clear everything.
	bitmap_set(bitmap, 0);
	bitmap_set(bitmap, 3);
	bitmap_set(bitmap, 71);
	bitmap_set(bitmap, 149);
	bitmap_clear_all(bitmap);
	check = check_clear();
	assert(check.empty(), check);

	assert(bitmap_first_set(bitmap) == -1, "empty bitmap reports set bit?");
	bitmap_set(bitmap, 0);
	assert(bitmap_first_set(bitmap) == 0, "bitmap [0] first set != 0?");
	bitmap_set(bitmap, 3);
	assert(bitmap_first_set(bitmap) == 0, "bitmap [0,3] first set != 0?");
	bitmap_set(bitmap, 65);
	assert(bitmap_first_set(bitmap) == 0, "bitmap [0,3,65] first set != 0?");
	bitmap_set(bitmap, 149);
	assert(bitmap_first_set(bitmap) == 0,
	       "bitmap [0,3,65,149] first set != 0?");
	bitmap_clear(bitmap, 0);
	assert(bitmap_first_set(bitmap) == 3,
	       "bitmap [3,65,149] first set != 3?");
	bitmap_clear(bitmap, 3);
	assert(bitmap_first_set(bitmap) == 65,
	       "bitmap [65,149] first set != 65?");
	bitmap_clear(bitmap, 65);
	assert(bitmap_first_set(bitmap) == 149, "bitmap [149] first set != 149?");
	bitmap_clear(bitmap, 149);
	assert(bitmap_first_set(bitmap) == -1, "empty bitmap reports set bit?");

	assert(bitmap_first_clear(bitmap) == 0,
	       "empty bitmap doesn't report 0 first clear bit?");
	bitmap_set(bitmap, 0);
	assert(bitmap_first_clear(bitmap) == 1, "bitmap [0] first clear != 1?");
	bitmap_set(bitmap, 1);
	assert(bitmap_first_clear(bitmap) == 2, "bitmap [0,1] first clear != 2?");
	bitmap_set(bitmap, 3);
	assert(bitmap_first_clear(bitmap) == 2,
	       "bitmap [0,1,3] first clear != 2?");
	bitmap_set(bitmap, 2);
	assert(bitmap_first_clear(bitmap) == 4,
	       "bitmap [0,1,2,3] first clear != 4?");

	// Now set all bits in all 3 words.
	bitmap_set_all(bitmap);

	// All expected bits should be set.
	for (int i = 0; i < 150; i++) {
		if (!bitmap_is_set(bitmap, i)) {
			std::ostringstream oss;
			oss << "bit " << i << " clear, should be set?";
			assert(false, oss.str());
		}
	}
	// Bits in final words >= num_bits % 64 should be clear.
	for (int i = 150; i < 192; i++) {
		if (bitmap_is_set(bitmap, i)) {
			std::ostringstream oss;
			oss << "bit " << i << " set, should be clear?";
			assert(false, oss.str());
		}
	}

	// Temporarily make the bitmap 2 words long to ensure the edge case works correctly.
	bitmap_clear_all(bitmap);
	bitmap->num_bits = 128;
	bitmap->num_words = 2;

	bitmap_set_all(bitmap);
	for (int i = 0; i < 128; i++) {
		if (!bitmap_is_set(bitmap, i)) {
			std::ostringstream oss;
			oss << "bit " << i << " clear, should be set?";
			assert(false, oss.str());
		}
	}

	// Restore sanity.
	bitmap_clear_all(bitmap);
	bitmap->num_bits = 150;
	bitmap->num_words = 3;

	// Make sure we don't return an out-of-range bit.
	bitmap_set_all(bitmap);
	assert(bitmap_first_clear(bitmap) == -1,
	       "Fully set bitmap first clear != -1?");

	// Now clear each individual bit and make sure bitmap_first_clear()
	// picks each index up.
	for (int i = 0; i < 150; i++) {
		bitmap_clear(bitmap, i);
		int64_t first_clear = bitmap_first_clear(bitmap);
		if (first_clear != i) {
			std::ostringstream oss;
			oss << "bitmap_first_clear() for bitmap with only bit "
			    << i << " clear reports " << first_clear
			    << " not the expected " << i;

			assert(false, oss.str());
		}
		bitmap_set(bitmap, i);
	}

	bitmap_set_all(bitmap);
	assert(bitmap_zero_string_longer_than(bitmap, 1) == -1,
	       "Fully set bitmap reports non-exist zero bitstring?");

	bitmap_clear(bitmap, 137);
	assert(bitmap_zero_string_longer_than(bitmap, 1) == 137,
	       "Bitmap zero string of length 1 not detected?");

	bitmap_clear(bitmap, 138);
	assert(bitmap_zero_string_longer_than(bitmap, 2) == 137,
	       "Not found zero string of size 2?");

	bitmap_clear(bitmap, 139);
	assert(bitmap_zero_string_longer_than(bitmap, 3) == 137,
	       "Not found zero string of size 3?");

	assert(bitmap_zero_string_longer_than(bitmap, 4) == -1,
	       "Only bitstring of length 3 exists, but returns index?");

	bitmap_clear(bitmap, 149);
	bitmap_clear(bitmap, 148);
	bitmap_clear(bitmap, 147);
	bitmap_clear(bitmap, 146);
	assert(bitmap_zero_string_longer_than(bitmap, 4) == 146,
	       "Not finding 4 length bitstring at index 146?");

	bitmap_set(bitmap, 147);
	assert(bitmap_zero_string_longer_than(bitmap, 4) == -1,
	       "Overlapping into out of range zero bits?");

	return "";
}
