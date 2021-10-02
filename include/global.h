#pragma once

#include "page.h"
#include "spinlock.h"

// Represents which stage the kernel is currently at and therefore what
// facilities are available.
enum kernel_stage {
	KERNEL_STAGE_1_EARLY = 1,
	KERNEL_STAGE_2_HAS_RING_BUFFER = 2,
	KERNEL_STAGE_3_HAS_INTERRUPT_HANDLER = 3,
	KERNEL_STAGE_4_HAS_MULTI_CORE = 4,
	KERNEL_STAGE_5_LOADED = 5,
};

// Represents global kernel state.
struct kernel_global {
	enum kernel_stage stage;

	spinlock_t lock;
};
static_assert(sizeof(struct kernel_global) < PAGE_SIZE);

// Initiailise kernel global state. `ptr` points at an allocated single page of
// memory.
void global_init(void *ptr);

// Retrieve global kernel state, acquiring a spinlock on it.
struct kernel_global *global_get_locked(void);
