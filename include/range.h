#pragma once

// Types and functions for interacting with ranges of values.

typedef struct {
	uint64_t from, to; // INCLUSIVE range, so of non-zero size.
} range_pair_t;

typedef enum {
	RANGE_SEPARATE, // Ranges are entirely separate.
	RANGE_ADJACENT, // Ranges are separate but adjacent.
	RANGE_OVERLAP,	// Ranges overlap.
	RANGE_COINCIDE, // One range fully contains the other.
} range_interact_t;

// Is value `n` contained within `range`?
static inline bool range_within(range_pair_t range, uint64_t n)
{
	return n >= range.from && n <= range.to;
}

// Compare 2 ranges of uint64_t values to determine whether they are separate,
// adjacent, overlap or coincide.
static inline range_interact_t range_compare(range_pair_t a, range_pair_t b)
{
	bool a_from = range_within(b, a.from);
	bool a_to = range_within(b, a.to);
	bool b_from = range_within(a, b.from);
	bool b_to = range_within(a, b.to);

	// ....     ..       ....    ....
	//  ##  or #### or   ##   or   ##
	if ((a_from && a_to) || (b_from && b_to))
		return RANGE_COINCIDE;

	// ....        ....
	//   #### or ####
	else if (b_from || b_to)
		return RANGE_OVERLAP;

	// ....            ....
	//     #### or ####
	if (a.to + 1 == b.from || b.to + 1 == a.from)
		return RANGE_ADJACENT;

	// ....                  ....
	//      ~ #### or #### ~
	return RANGE_SEPARATE;
}
