#include "test_user.h"

#include "compiler.h"
#undef static_assert // zeptux breaks static_assert in c++ :)

#include <sstream>
#include <string>

namespace {
std::string assert_bit_ops_correct()
{
	uint64_t x = 0;
	assert(find_first_set_bit(x) == -1, "find_first_set_bit(0) != -1?");
	// Test each individual bit.
	for (int i = 0; i < 64; i++) {
		uint64_t mask = 1UL << i;
		x |= mask;

		int64_t bit = find_first_set_bit(x);
		if (bit != i) {
			std::ostringstream oss;
			oss << "bit " << i
			    << " set but find_first_set_bit() == " << bit;
			assert(false, oss.str());
		}

		x ^= mask;
	}

	x = ~0UL;
	assert(find_first_clear_bit(x) == -1, "find_first_clear_bit(~0) != -1?");
	// Test each individual bit.
	for (int i = 0; i < 64; i++) {
		uint64_t mask = 1UL << i;
		x ^= mask;

		int64_t bit = find_first_clear_bit(x);
		if (bit != i) {
			std::ostringstream oss;
			oss << "bit " << i
			    << " cleared but find_first_clear_bit() == " << bit;
			assert(false, oss.str());
		}

		x |= mask;
	}

	return "";
}
} // namespace

std::string test_misc()
{
	std::string res = assert_bit_ops_correct();
	if (!res.empty())
		return res;

	return "";
}
