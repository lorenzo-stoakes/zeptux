// Needed to fake out the stdarg zeptux kernel replacements.
#include <cstdint>

#include "range.h"
#include "test_user.h"

#include <sstream>
#include <string>

const std::string test_range(void)
{
	assert(range_within({0, 0}, 0), "0 not within [0,0]?");
	assert(!range_within({0, 0}, 1), "1 within [0,0]?");

	assert(range_within({0, 1}, 0), "0 not within [0,1]?");
	assert(range_within({0, 1}, 1), "1 not within [0,1]?");
	assert(!range_within({0, 1}, 2), "2 within [0,1]?");

	assert(range_compare({0, 0}, {2, 2}) == RANGE_SEPARATE,
	       "[0,0]; [2,2] aren't separate?");
	assert(range_compare({2, 2}, {0, 0}) == RANGE_SEPARATE,
	       "[2,2]; [0,0] aren't separate?");
	assert(range_compare({10, 15}, {17, 19}) == RANGE_SEPARATE,
	       "[10,15]; [17,19] aren't separate?");
	assert(range_compare({17, 19}, {10, 15}) == RANGE_SEPARATE,
	       "[17,19]; [10,15] aren't separate?");

	assert(range_compare({0, 0}, {1, 1}) == RANGE_ADJACENT,
	       "[0,0]; [1,1] aren't adjacent?");
	assert(range_compare({1, 1}, {0, 0}) == RANGE_ADJACENT,
	       "[1,1]; [0,0] aren't adjacent?");

	assert(range_compare({17, 193}, {194, 2019}) == RANGE_ADJACENT,
	       "[17,193]; [194,2019] aren't adjacent?");
	assert(range_compare({194, 2019}, {17, 193}) == RANGE_ADJACENT,
	       "[194,2019]; [17,193] aren't adjacent?");

	// Check coincide logic.
	for (uint64_t from_a = 0; from_a <= 10; from_a++) {
		for (uint64_t to_a = from_a; to_a <= 10; to_a++) {
			for (uint64_t from_b = from_a; from_b <= to_a;
			     from_b++) {
				for (uint64_t to_b = from_b; to_b <= to_a;
				     to_b++) {
					std::ostringstream oss;
					oss << "[" << from_a << "," << to_a;
					oss << "]; [" << from_b << "," << to_b;
					oss << "] don't coincide?";

					assert(range_compare({from_a, to_a},
							     {from_b, to_b}) ==
						       RANGE_COINCIDE,
					       oss.str());

					oss << " (reversed)";
					assert(range_compare({from_b, to_b},
							     {from_a, to_a}) ==
						       RANGE_COINCIDE,
					       oss.str());
				}
			}
		}
	}

	// Check overlap logic.
	for (uint64_t from_a = 0; from_a <= 9; from_a++) {
		// Have to be of size 2 or more for overlap to be possible.
		for (uint64_t to_a = from_a + 1; to_a <= 10; to_a++) {
			for (uint64_t from_b = from_a + 1; from_b <= to_a;
			     from_b++) {
				for (uint64_t to_b = to_a + 1; to_b <= 11;
				     to_b++) {
					std::ostringstream oss;
					oss << "[" << from_a << "," << to_a;
					oss << "]; [" << from_b << "," << to_b;
					oss << "] don't overlap?";

					assert(range_compare({from_a, to_a},
							     {from_b, to_b}) ==
						       RANGE_OVERLAP,
					       oss.str());

					oss << " (reversed)";
					assert(range_compare({from_b, to_b},
							     {from_a, to_a}) ==
						       RANGE_OVERLAP,
					       oss.str());
				}
			}
		}
	}

	return "";
}
