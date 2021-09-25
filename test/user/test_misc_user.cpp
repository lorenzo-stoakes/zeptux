#include "test_user.h"

#include "compiler.h"
#undef static_assert // zeptux breaks static_assert in c++ :)
#include "list.h"

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

struct elem {
	int x, y;
	struct list_node node;
	int z;
};

std::string assert_list_correct()
{
	LIST_DEFINE(list);

	assert(list_empty(&list), "Non-empty initial list?");
	assert(list_count(&list) == 0, "Non-empty inital list?");

	for_each_list_element (&list, dummy, struct elem, node) {
		assert(false, "Can iterate empty list?");
	}

	for (int i = 0; i < 10; i++) {
		elem *ptr = new elem();
		ptr->x = i;
		ptr->y = i + 1;
		ptr->z = i + 2;

		list_push_back(&list, &ptr->node);

		assert(list_count(&list) == (uint64_t)(i + 1),
		       "List entry not added?");

		assert(list_last_element(&list, struct elem, node) == ptr,
		       "Just added element not last element?");
	}
	assert(!list_empty(&list), "Populated list empty?");

	struct list_node *tmp;
	struct elem *el;
	int i = 0;
	for_each_list_element_safe (&list, el, tmp, node) {
		assert(el->x == i, "x != i?");
		assert(el->y == i + 1, "y != i + 1?");
		assert(el->z == i + 2, "z != i + 2?");

		// Remove element from list.
		list_detach(&el->node);
		delete el;

		assert(list_count(&list) == 10 - (uint64_t)i - 1,
		       "Element not deleted?");

		i++;
	}
	assert(i == 10, "Iterated through <10 times?");
	assert(list_empty(&list), "Deleted every element and list not empty?");

	// Insert to front of list.
	for (int i = 0; i < 10; i++) {
		elem *ptr = new elem();
		ptr->x = 10 - i;

		list_push_front(&list, &ptr->node);

		assert(list_count(&list) == (uint64_t)(i + 1),
		       "List entry not added?");

		assert(list_first_element(&list, struct elem, node) == ptr,
		       "Just added element not first element?");
	}

	// Assert that they were inserted correctly.
	i = 0;
	for_each_list_element (&list, el, struct elem, node) {
		assert(el->x == i + 1, "Not inserted correctly?");
		i++;
	}

	// Insert entries between nodes.
	i = 0;
	for_each_list_element_safe (&list, el, tmp, node) {
		elem *ptr = new elem();
		ptr->x = 99;

		if (i % 2 == 0)
			list_node_insert_after(&el->node, &ptr->node);
		else
			list_node_insert_before(&el->node, &ptr->node);

		i++;
	}

	assert(list_count(&list) == 20, "Not correctly inserted extra fields");

	i = 1;
	int j = 0;
	for_each_list_element_safe (&list, el, tmp, node) {
		if (el->x == 99) {
			assert(j < 2, "More 99's than expected?");
			j++;
		} else {
			j = 0;
			assert(el->x == i, "x != i?");
			i++;
		}
	}

	return "";
}

} // namespace

std::string test_misc()
{
	std::string res = assert_bit_ops_correct();
	if (!res.empty())
		return res;

	res = assert_list_correct();
	if (!res.empty())
		return res;

	return "";
}
