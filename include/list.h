#pragma once

#include "compiler.h"
#include "consts.h"
#include "macros.h"
#include "types.h"

// This implementation obviously bears some resemblance to the linux linked list
// implementation from which obviously inspiration was taken. Good ideas don't
// die.

// We place a list_node within the struct we wish to keep in a list which links
// it to other elements within the list. The list is a ring so we can avoid
// branches for NULL checks and updating list head.

// Represents a doubly-linked list. This is structurally identical to a
// list_node but typed differently to explicitly seperate a representation of a
// list from that of its elements.
struct list {
	struct list_node *last, *first;
};

// Represents a doubly-linked list entry.
struct list_node {
	struct list_node *prev, *next;
};
static_assert(sizeof(struct list) == sizeof(struct list_node) &&
	      offsetof(struct list, last) == offsetof(struct list_node, prev) &&
	      offsetof(struct list, first) == offsetof(struct list_node, next));

// Initial list value, initialise a list by assigning to this value.
#define LIST_INIT_VALUE(_list_name)                       \
	{                                                 \
		(struct list_node *)&(_list_name),        \
			(struct list_node *)&(_list_name) \
	}

// Define a newly initialised list.
#define LIST_DEFINE(_list_name) \
	struct list _list_name = LIST_INIT_VALUE(_list_name)

// Initialise a list node as empty.
static inline void list_node_init(struct list_node *node)
{
	node->next = node;
	node->prev = node;
}

// Initialise a list as empty.
static inline void list_init(struct list *list)
{
	struct list_node *node = (struct list_node *)list;
	list_node_init(node);
}

// Determine if a list is empty.
static inline bool list_empty(struct list *list)
{
	return list->first == (struct list_node *)list;
}

// Obtain the element containing a given list node.
#define list_node_element(_list_node_ptr, _type, _member) \
	container_of((_list_node_ptr), _type, _member)

// Obtain a pointer to the element at the front of the list. List must be
// non-empty!
#define list_first_element(_list_ptr, _type, _member) \
	list_node_element((_list_ptr)->first, _type, _member)

// Obtain a pointer to the element at the last of the list. List must be
// non-empty!
#define list_last_element(_list_ptr, _type, _member) \
	list_node_element((_list_ptr)->last, _type, _member)

// Iterates through each element in a list pointed to by `_list_ptr`,
// instantiating a block scope variable called `_elem_name` with element type
// `_type` and list_node member in each element `_member`.
#define for_each_list_element(_list_ptr, _elem_name, _type, _member)         \
	for (_type *_elem_name =                                             \
		     list_first_element((_list_ptr), _type, _member);        \
	     &_elem_name->_member != (struct list_node *)(_list_ptr);        \
	     _elem_name = list_node_element(_elem_name->_member.next, _type, \
					    _member))

// Iterates safely (i.e. deletion of nodes can occur) through each element in a
// list pointed to by `_list_ptr`, using element variable `_elem`, and struct
// list_node * `_tmp` with list_node  member in each element `_member`.
#define for_each_list_element_safe(_list_ptr, _elem, _tmp, _member)            \
	for (_elem = list_first_element((_list_ptr), typeof(*_elem), _member), \
	    _tmp = (_list_ptr)->first->next;                                   \
	     &_elem->_member != (struct list_node *)(_list_ptr);               \
	     _elem = list_node_element(_tmp, typeof(*_elem), _member),         \
	    _tmp = _tmp->next)

// Insert node `it` before `node`.
static inline void list_node_insert_before(struct list_node *node,
					   struct list_node *it)
{
	// [ prev ] <->              [ node ]
	struct list_node *prev = node->prev;
	//              [  it  ] <-  [ node ]
	node->prev = it;
	// [ prev ]  -> [  it  ] <-  [ node ]
	prev->next = it;
	// [ prev ] <-> [  it  ] <-  [ node ]
	it->prev = prev;
	// [ prev ] <-> [  it  ] <-> [ node ]
	it->next = node;
}

// Insert node `it` after `node`.
static inline void list_node_insert_after(struct list_node *node,
					  struct list_node *it)
{
	// [ node ] <->              [ next ]
	struct list_node *next = node->next;
	// [ node ]  -> [  it  ]
	node->next = it;
	// [ node ]  -> [  it  ] <-  [ next ]
	next->prev = it;
	// [ node ] <-> [  it  ] <-  [ next ]
	it->prev = node;
	// [ node ] <-> [  it  ] <-> [ next ]
	it->next = next;
}

// Remove node from list.
static inline void list_detach(struct list_node *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	// Make it part of an isolated, empty list.
	node->prev = node;
	node->next = node;
}

// Insert node `it` at end of list.
static inline void list_push_back(struct list *list, struct list_node *it)
{
	list_node_insert_before((struct list_node *)list, it);
}

// Insert node `it` at front of list.
static inline void list_push_front(struct list *list, struct list_node *it)
{
	list_node_insert_after((struct list_node *)list, it);
}

// Count the number of elements in the list.
// NOTE: This is O(n) so should be avoided unless absolutely necessary.
static inline uint64_t list_count(struct list *list)
{
	uint64_t ret = 0;

	for (struct list_node *node = list->first;
	     node != (struct list_node *)list; node = node->next) {
		ret++;
	}

	return ret;
}
