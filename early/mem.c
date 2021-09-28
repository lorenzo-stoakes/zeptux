#include "zeptux_early.h"

enum early_alloc_type {
	EARLY_ALLOC_NORMAL,
	EARLY_ALLOC_EPHEMERAL,
	EARLY_ALLOC_PAGETABLE,
	EARLY_ALLOC_PHYSBLOCK,
};

// Stores scratch allocator state. We will only access this single-threaded so
// it's ok to keep this static.
static struct scratch_alloc_state scratch_state;

// We allocate pages for this using the scratch allocator but keep the pointer
// as a static pointer. We will only access this single-threaded also.
static struct early_page_alloc_state *alloc_state;

// Specify the allocators to use in page mapping.
static struct page_allocators early_allocators = {
	.pud = early_alloc_pud,
	.pmd = early_alloc_pmd,
	.ptd = early_alloc_ptd,
	.data = early_page_alloc_zero,

	.panic = _early_panic,
};

// Drop the direct mapping from VA 0 / PA 0. We don't need it any more.
static void drop_direct0(void)
{
	memset((void *)EARLY_PUD_DIRECT0_ADDRESS, 0, PAGE_SIZE);
	memset((void *)EARLY_PGD_ADDRESS, 0, sizeof(uint64_t));
	global_flush_tlb();
}

// Compare 2 e820 entries and determine which one is 'less' than the other. If
// the bases are equal we tie-break on size.
static bool less(struct e820_entry *a, struct e820_entry *b)
{
	return a->base < b->base || (a->base == b->base && a->size < b->size);
}

// Initialise the early scratch allocator, a very very simplistic bootstrap
// allocator that allows up to EARLY_MAX_SCRATCH_PAGES pages to be allocated,
// which are placed before the start ELF image. These pages will _not_ be kept
// later.
static void early_scratch_alloc_init()
{
	// We place the scratch buffer immediately prior to the ELF.
	uint64_t offset =
		KERNEL_ELF_ADDRESS_PHYS - (EARLY_MAX_SCRATCH_PAGES << PAGE_SHIFT);
	physaddr_t addr = {offset};
	scratch_state.start = addr;
	scratch_state.pages = 0;
}

// Delete an entry at `index` in the e820 array. This is O(n).
static void delete_e820_entry(struct early_boot_info *info, int index)
{
	// This should never happen but for the sake of avoiding underflow,
	// check for it.
	if (info->num_e820_entries == 0)
		early_panic("0 e820 entries?");

	for (int i = index; i < (int)info->num_e820_entries - 1; i++) {
		info->e820_entries[i] = info->e820_entries[i + 1];
	}
	info->num_e820_entries--;
}

void early_sort_e820(struct early_boot_info *info)
{
	// Insertion sort both because n is small and it's highly likely this
	// will already be sorted which would make this O(n).
	for (int i = 1; i < (int)info->num_e820_entries; i++) {
		struct e820_entry key = info->e820_entries[i];

		int j;
		for (j = i - 1; j >= 0 && !less(&info->e820_entries[j], &key);
		     j--) {
			info->e820_entries[j + 1] = info->e820_entries[j];
		}
		info->e820_entries[j + 1] = key;
	}
}

void early_merge_e820(struct early_boot_info *info)
{
	// Note that we assume that all zero-size entries have been pruned out
	// by the boot code AND that the entries have been sorted.

	int curr_index = 0;
	for (int i = 1; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *curr = &info->e820_entries[curr_index];
		struct e820_entry *entry = &info->e820_entries[i];
		range_pair_t curr_range = {
			.from = curr->base,
			.to = curr->base + curr->size - 1,
		};
		range_pair_t entry_range = {
			.from = entry->base,
			.to = entry->base + entry->size - 1,
		};

		if (entry->type != curr->type)
			goto keep;

		// We've sorted the arrays so we know which order the ranges are
		// in.
		switch (range_compare(curr_range, entry_range)) {
		case RANGE_SEPARATE:
			// ....
			//      ~ ####
			break;
		case RANGE_ADJACENT:
		case RANGE_OVERLAP:
			// ....        ....
			//     #### or   ####
			curr->size += entry_range.to - curr_range.to;
			goto loop;
		case RANGE_COINCIDE:
			// ......
			//  ####
			goto loop;
		}

	keep:
		curr_index++;
		if (curr_index != i)
			info->e820_entries[curr_index] = *entry;

	loop:
	}

	info->num_e820_entries = curr_index + 1;
}

void early_normalise_e820(struct early_boot_info *info)
{
	// Since we can only map page-aligned physical addresses those parts of
	// available physical memory that start or end unaligned are simply not
	// safely available to us. To ensure we only access available memory,
	// page align all RAM entries.
	for (int i = 0; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *entry = &info->e820_entries[i];

		if (entry->type != E820_TYPE_RAM)
			continue;

		uint64_t start = entry->base;
		uint64_t end = entry->base + entry->size; // Exclusive.

		// Most likely situation - we are already page-aligned and so
		// have nothing to do.
		if (IS_ALIGNED(start, PAGE_SIZE) && IS_ALIGNED(end, PAGE_SIZE))
			continue;

		// Align start to page size, rounding up so we don't access
		// unavailable memory.
		start = ALIGN_UP(start, PAGE_SIZE);
		// Align end to page size, rounding down so we don't access
		// unavailable memory.
		end = ALIGN(end, PAGE_SIZE);

		// We had to page-align, but the entry wasn't so tiny that it
		// meant a page-aligned page of data wasn't available.
		if (start < end) {
			entry->base = start;
			entry->size = end - start;
			continue;
		}

		// This is rather unlikely but conceivable - the entry is so
		// small that it doesn't have a page-aligned page of data
		// available, so we have to delete it.

		// If we had to do this for enough entries this would be
		// O(n^2). However this scenario is unlikely and n is small in
		// any case.
		delete_e820_entry(info, i);
		i--;
	}
}

void early_set_total_ram(struct early_boot_info *info)
{
	// Note that we assume we have sorted, merged and normalised e820
	// entries at this stage, meaning the total ram figure will be for
	// page-aligned physical memory and be a multiple of the page size.

	uint64_t bytes = 0;

	uint64_t num_ram_entries = 0;
	for (int i = 0; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *entry = &info->e820_entries[i];

		if (entry->type != E820_TYPE_RAM)
			continue;

		bytes += entry->size;
		num_ram_entries++;
	}

	if (num_ram_entries > MAX_E820_RAM_ENTRIES)
		early_panic(
			"There are %lu E820 RAM entries present, maximum permitted is %lu",
			num_ram_entries, MAX_E820_RAM_ENTRIES);

	info->total_avail_ram_bytes = bytes;
	// There is a one-to-one mapping between e820 RAM entries and spans of
	// physical RAM.
	info->num_ram_spans = num_ram_entries;
}

struct scratch_alloc_state *early_scratch_alloc_state(void)
{
	return &scratch_state;
}

physaddr_t early_scratch_page_alloc(void)
{
	// About as simplistic as it gets.

	if (scratch_state.pages >= EARLY_MAX_SCRATCH_PAGES)
		early_panic("Out of scratch memory!");

	physaddr_t addr = {scratch_state.start.x +
			   PAGE_SIZE * scratch_state.pages++};

	// Zero the page.
	uint8_t *ptr = phys_to_virt_ptr(addr);
	memset(ptr, 0, PAGE_SIZE);

	return addr;
}

// Allocates bitmaps for early allocator span data, assigns the spans,
// initialises them and returns the number of pages allocated.
static uint64_t alloc_span_bitmaps(struct early_page_alloc_span *span)
{
	// The number of pages this bitmap will keep track of.
	uint64_t num_pages = span->num_pages;
	// The number of bytes/pages the actual bitmap itself will occupy.
	uint64_t bitmap_bytes = bitmap_calc_size(num_pages);
	uint64_t bitmap_pages = bytes_to_pages(bitmap_bytes);

	// We rely on the fact that the scratch allocator is giving us
	// physically (and this via direct mapping virtually) contiguous memory.
	span->alloc_bitmap = phys_to_virt_ptr(early_scratch_page_alloc());
	for (int i = 1; i < (int)bitmap_pages; i++) {
		early_scratch_page_alloc();
	}

	span->ephemeral_bitmap = phys_to_virt_ptr(early_scratch_page_alloc());
	for (int i = 1; i < (int)bitmap_pages; i++) {
		early_scratch_page_alloc();
	}

	span->pagetable_bitmap = phys_to_virt_ptr(early_scratch_page_alloc());
	for (int i = 1; i < (int)bitmap_pages; i++) {
		early_scratch_page_alloc();
	}

	span->physblock_bitmap = phys_to_virt_ptr(early_scratch_page_alloc());
	for (int i = 1; i < (int)bitmap_pages; i++) {
		early_scratch_page_alloc();
	}

	bitmap_init(span->alloc_bitmap, num_pages);
	bitmap_init(span->ephemeral_bitmap, num_pages);
	bitmap_init(span->pagetable_bitmap, num_pages);
	bitmap_init(span->physblock_bitmap, num_pages);

	return bitmap_pages * 4;
}

// Find the early page allocator span that contains a specified physical
// address.
static struct early_page_alloc_span *find_span(physaddr_t pa)
{
	for (int i = 0; i < (int)alloc_state->num_spans; i++) {
		struct early_page_alloc_span *span = &alloc_state->spans[i];

		uint64_t start = span->start.x;
		if (start <= pa.x && pa.x < start + span->num_pages * PAGE_SIZE) {
			return span;
		}
	}

	return NULL;
}

// Determine the offset of a specific physical address into an early page
// allocator span in pages. Assumes the PA has already been located within the
// span via find_span() above.
static inline uint64_t phys_to_span_offset(struct early_page_alloc_span *span,
					   physaddr_t pa)
{
	uint64_t offset_bytes = pa.x - span->start.x;
	return offset_bytes >> PAGE_SHIFT;
}

// Determine the physical address of an offset into an early page allocator
// span. This assumes the offset is already known to be contained within the
// span.
static inline physaddr_t span_offset_to_pa(struct early_page_alloc_span *span,
					   uint64_t offset)
{
	physaddr_t pa = span->start;
	pa.x += offset << PAGE_SHIFT;

	return pa;
}

// Mark a page allocated and update stats accordingly in the early page
// allocator.
static void mark_page_allocated(struct early_page_alloc_span *span,
				uint64_t offset, enum early_alloc_type type)
{
	bitmap_set(span->alloc_bitmap, offset);
	alloc_state->num_allocated_pages++;
	span->num_allocated_pages++;

	switch (type) {
	case EARLY_ALLOC_NORMAL:
		break;
	case EARLY_ALLOC_EPHEMERAL:
		bitmap_set(span->ephemeral_bitmap, offset);
		alloc_state->num_ephemeral_pages++;
		break;
	case EARLY_ALLOC_PAGETABLE:
		bitmap_set(span->pagetable_bitmap, offset);
		alloc_state->num_pagetable_pages++;
		break;
	case EARLY_ALLOC_PHYSBLOCK:
		bitmap_set(span->physblock_bitmap, offset);
		alloc_state->num_physblock_pages++;
		break;
	default:
		early_panic("Impossible!");
	}
}

// Allocate at the specific physical address in the early page allocator or
// panic if already allocated. This will only ever be called single threaded so
// we don't have to worry about sychronisation.
static void alloc_at(physaddr_t pa, enum early_alloc_type type)
{
	struct early_page_alloc_span *span = find_span(pa);

	if (span == NULL) {
		early_panic("Unable to locate span for %lx", pa.x);
	}

	uint64_t offset = phys_to_span_offset(span, pa);
	if (bitmap_is_set(span->alloc_bitmap, offset)) {
		early_panic("Page containing %lx is already allocated?", pa.x);
	}

	mark_page_allocated(span, offset, type);
}

// Allocate a page from the early page allocator.
static physaddr_t alloc(enum early_alloc_type type)
{
	// TODO: Perhaps we should use some heuristic to avoid giving away too
	// much physically contiguous memory to allocations who might very well
	// not need it?

	for (int i = 0; i < (int)alloc_state->num_spans; i++) {
		struct early_page_alloc_span *span = &alloc_state->spans[i];

		int64_t offset = bitmap_first_clear(span->alloc_bitmap);
		if (offset == -1)
			continue;

		mark_page_allocated(span, offset, type);
		return span_offset_to_pa(span, offset);
	}

	early_panic("Out of memory, %lu pages of %lu allocated!",
		    alloc_state->num_allocated_pages, alloc_state->total_pages);
}

void early_page_alloc_ephemeral_at(physaddr_t pa)
{
	alloc_at(pa, EARLY_ALLOC_EPHEMERAL);
}

void early_page_alloc_at(physaddr_t pa)
{
	alloc_at(pa, EARLY_ALLOC_NORMAL);
}

physaddr_t early_page_alloc(void)
{
	return alloc(EARLY_ALLOC_NORMAL);
}

physaddr_t early_page_alloc_ephemeral(void)
{
	return alloc(EARLY_ALLOC_EPHEMERAL);
}

physaddr_t early_pagetable_alloc(void)
{
	physaddr_t pa = alloc(EARLY_ALLOC_PAGETABLE);
	zero_page(pa);
	return pa;
}

physaddr_t early_physblock_page_alloc(void)
{
	physaddr_t pa = alloc(EARLY_ALLOC_PHYSBLOCK);
	zero_page(pa);
	return pa;
}

void early_page_free(physaddr_t pa)
{
	struct early_page_alloc_span *span = find_span(pa);
	if (span == NULL)
		early_panic("Cannot find span for PA %lx", pa.x);

	uint64_t offset = phys_to_span_offset(span, pa);

	if (!bitmap_is_set(span->alloc_bitmap, offset)) {
		early_panic("Attempt to free unallocated PA %lx", pa.x);
	}

	bitmap_clear(span->alloc_bitmap, offset);
	alloc_state->num_allocated_pages--;
	span->num_allocated_pages--;

	if (bitmap_is_set(span->ephemeral_bitmap, offset)) {
		bitmap_clear(span->ephemeral_bitmap, offset);
		alloc_state->num_ephemeral_pages--;
	}

	if (bitmap_is_set(span->pagetable_bitmap, offset)) {
		bitmap_clear(span->pagetable_bitmap, offset);
		alloc_state->num_pagetable_pages--;
	}

	if (bitmap_is_set(span->physblock_bitmap, offset)) {
		bitmap_clear(span->physblock_bitmap, offset);
		alloc_state->num_physblock_pages--;
	}
}

// Allocate ephemeral pages allocated from the scratch allocator in order to
// initialise the early page allocator as well as system memory already being
// used such as the kernel stack in order that these pages are reserved.
static void alloc_init_pages(physaddr_t first_scratch_page,
			     uint64_t num_scratch_pages)
{
	// Allocate (ephemeral) scratch pages.
	physaddr_t pa = first_scratch_page;
	for (int i = 0; i < (int)num_scratch_pages; i++) {
		early_page_alloc_ephemeral_at(pa);
		pa = phys_next_page(pa);
	}

	// Allocate kernel stack.
	pa.x = KERNEL_STACK_ADDRESS_PHYS;
	for (int i = 0; i < KERNEL_STACK_PAGES; i++) {
		early_page_alloc_at(pa);
		pa = phys_prev_page(pa);
	}

	// Alloc early boot info.
	struct early_boot_info *info = early_get_boot_info();
	pa = virt_ptr_to_phys(info);
	early_page_alloc_at(pa);

	// Allocate ELF image pages.
	pa.x = KERNEL_ELF_ADDRESS_PHYS;
	uint64_t elf_pages = bytes_to_pages(info->kernel_elf_size_bytes);
	for (int i = 0; i < (int)elf_pages; i++) {
		early_page_alloc_at(pa);
		pa = phys_next_page(pa);
	}

	// Allocate (ephemeral) early page tables - we will remap the page
	// tables before moving to the full fat physical allocator.
	pa.x = X86_EARLY_PGD;
	early_page_alloc_ephemeral_at(pa);
	// We have already removed the direct 0 mapping so no need to allocate
	// that.
	pa.x = X86_EARLY_PUD_DIRECT_MAP;
	early_page_alloc_ephemeral_at(pa);
	pa.x = X86_EARLY_PUD_KERNEL_ELF;
	early_page_alloc_ephemeral_at(pa);

	// Allocate (should be ephemeral) early video buffer address.
	// TODO: Make ephemeral when we have a proper video driver.
	pa.x = X86_EARLY_VIDEO_BUFFER_ADDRESS_PHYS;
	early_page_alloc_at(pa);
	// The early video buffer address is very slightly bigger than 1 page so
	// we must allocate the next page also!
	pa.x += 0x1000;
	early_page_alloc_at(pa);
}

void early_page_alloc_init(struct early_boot_info *info)
{
	// Allocate the page that contains the early page alloc metadata and all
	// span metadata. As we know that the scratch allocator is allocating
	// physically contiguous memory we need only track the 1st address and
	// number allocated to know what we have used.
	physaddr_t first_scratch_page = early_scratch_page_alloc();
	uint64_t num_scratch_pages = 1;

	if (!IS_ALIGNED(info->total_avail_ram_bytes, PAGE_SIZE))
		early_panic("e820 entries not normalised to page size?");

	alloc_state = phys_to_virt_ptr(first_scratch_page);
	alloc_state->total_pages = bytes_to_pages(info->total_avail_ram_bytes);
	alloc_state->num_spans = info->num_ram_spans;

	uint64_t span_index = 0;
	for (int i = 0; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *entry = &info->e820_entries[i];

		if (entry->type != E820_TYPE_RAM)
			continue;

		struct early_page_alloc_span *span =
			&alloc_state->spans[span_index++];

		physaddr_t start = {entry->base};
		span->start = start;
		span->num_pages = entry->size >> PAGE_SHIFT;

		num_scratch_pages += alloc_span_bitmaps(span);
	}

	alloc_init_pages(first_scratch_page, num_scratch_pages);
}

struct early_page_alloc_state *early_get_page_alloc_state(void)
{
	return alloc_state;
}

void early_map_direct(struct early_boot_info *info, pgdaddr_t pgd)
{
	for (int i = 0; i < (int)info->num_e820_entries; i++) {
		struct e820_entry *entry = &info->e820_entries[i];

		if (entry->type != E820_TYPE_RAM)
			continue;

		physaddr_t pa = {entry->base};
		virtaddr_t va = phys_to_virt(pa);
		uint64_t num_pages = bytes_to_pages(entry->size);

		_map_page_range(pgd, va, pa, num_pages, MAP_KERNEL,
				&early_allocators);
	}
}

void early_map_kernel_elf(struct elf_header *header, physaddr_t elf_pa,
			  pgdaddr_t pgd)
{
	// Map kernel ELF, marking sections accordingly depending on whether
	// they contain code or not, are readonly or not, etc.

	virtaddr_t va = {(uint64_t)header};
	if (!IS_ALIGNED(va.x, PAGE_SIZE))
		early_panic("Misaligned kernel ELF header by %lu",
			    va.x % PAGE_SIZE);

	// Now work through each section, mapping accordingly.
	struct elf_section_header *sect_headers = (void *)header + header->shoff;

	physaddr_t pa;
	for (int i = 0; i < (int)header->shnum; i++) {
		struct elf_section_header *sect_header = &sect_headers[i];

		// If not intended to be mapped into memory, ignore.
		if (sect_header->addr == 0)
			continue;

		va.x = sect_header->addr;

		// If the section does not actually occupy memory itself
		// (e.g. .bss), we have to allocate memory for it.
		if (sect_header->type == ELF_SHT_NOBITS) {
			if (sect_header->size > PAGE_SIZE)
				early_panic(
					"Kernel ELF NOBITS header exceeds page size");

			pa = early_page_alloc();
			// We need to copy the current state of the header into
			// the page.
			// NOTE: We copy the full page as ELF section headers
			// can overlap with .bss for example and not doing so
			// loses this data.
			memcpy(phys_to_virt_ptr(pa), (void *)sect_header->addr,
			       PAGE_SIZE);
		} else {
			pa.x = elf_pa.x + sect_header->offset;
		}

		// We insist on page-aligned ELF sections.
		if (!IS_ALIGNED(va.x, PAGE_SIZE) || !IS_ALIGNED(pa.x, PAGE_SIZE))
			early_panic("Misaligned kernel ELF section (%d) by %lu",
				    i, va.x % PAGE_SIZE);

		// Determine mapping flags.
		map_flags_t flags = MAP_KERNEL;
		if (!IS_MASK_SET(sect_header->flags, ELF_SHF_WRITE))
			flags |= MAP_READONLY;
		if (IS_MASK_SET(sect_header->flags, ELF_SHF_EXECINSTR))
			flags |= MAP_CODE;

		// Do actual mapping.
		_map_page_range(pgd, va, pa, bytes_to_pages(sect_header->size),
				flags, &early_allocators);
	}

	// Map the header. It will never be larger than 1 page in size as we
	// have established it is page aligned. If already mapped in some other
	// section above, we skip it + retain attributes specified above.
	va.x = (uint64_t)header;
	_map_page_range(pgd, va, elf_pa, 1,
			MAP_KERNEL | MAP_READONLY | MAP_SKIP_IF_MAPPED,
			&early_allocators);

	// Map the section headers. It will never span more than 2 pages. These
	// might have already been mapped above, set map flag such that we skip
	// the mapping here if so.
	va.x += header->shoff;
	pa.x = elf_pa.x + header->shoff;

	_map_page_range(pgd, va, pa, 2,
			MAP_KERNEL | MAP_READONLY | MAP_SKIP_IF_MAPPED,
			&early_allocators);
}

void map_early_video(pgdaddr_t pgd)
{
	uint64_t num_pages = bytes_to_pages(early_video_size());
	virtaddr_t va = {EARLY_VIDEO_MEMORY_ADDRESS};
	physaddr_t pa = {EARLY_VIDEO_MEMORY_ADDRESS_PHYS};

	_map_page_range(pgd, va, pa, num_pages, MAP_DEVICE, &early_allocators);
}

void early_remap_page_tables(struct early_boot_info *info)
{
	physaddr_t elf_pa = {KERNEL_ELF_ADDRESS_PHYS};
	struct elf_header *header = (struct elf_header *)KERNEL_ELF_ADDRESS;
	pgdaddr_t pgd = early_alloc_pgd();

	early_map_direct(info, pgd);
	early_map_kernel_elf(header, elf_pa, pgd);
	map_early_video(pgd);

	set_pgd(pgd);
	kernel_root_pgd = pgd;
}

// Allocate and map struct physblock objects representing `num_pages` pages from
// `start`.
static void alloc_map_physblock(physaddr_t start, uint64_t num_pages)
{
	// We have to map the page _containing_ the first VA in the range up to
	// and including the page _containing_ the last VA in the range.
	uint64_t start_offset = phys_to_pfn(start).x * sizeof(struct physblock);
	uint64_t start_addr = KERNEL_MEM_MAP_ADDRESS + start_offset;
	uint64_t end_addr = start_addr + num_pages * sizeof(struct physblock);
	virtaddr_t start_va = {ALIGN(start_addr, PAGE_SIZE)};
	virtaddr_t end_va = {ALIGN_UP(end_addr, PAGE_SIZE)};

	for (virtaddr_t va = start_va; va.x < end_va.x; va = virt_next_page(va)) {
		// We initialise all physblocks to zero.
		physaddr_t pa = early_physblock_page_alloc();

		_map_page_range(kernel_root_pgd, va, pa, 1, MAP_KERNEL,
				&early_allocators);
	}
}

// Determine the physical address of memory referenced by an offset into an
// early page allocator span.
static physaddr_t span_to_phys(struct early_page_alloc_span *span, uint64_t i)
{
	physaddr_t pa = {span->start.x + PAGE_SIZE * i};

	return pa;
}

// Initialise the physblock for the early page allocated span. Assumes
// physblocks allocated and mapped.
static void init_physblock_span(struct early_page_alloc_span *span)
{
	// Everything is order-0 to begin with (the early page allocator cannot
	// allocate order > 0 pages) so we don't need to do anything special to
	// handle order > 0 physblocks.
	for (uint64_t i = 0; i < span->num_pages; i++) {
		physaddr_t pa = span_to_phys(span, i);
		struct physblock *block = phys_to_physblock_lock(pa);

		list_node_init(&block->node);

		// Non-allocated blocks are correctly specified by otherwise
		// zeroed initial data.
		if (!bitmap_is_set(span->alloc_bitmap, i))
			goto next;

		// Ephemeral pages need not be assigned as they will not be
		// added to the physical allocator.
		if (bitmap_is_set(span->ephemeral_bitmap, i))
			goto next;

		if (bitmap_is_set(span->pagetable_bitmap, i)) {
			block->type = PHYSBLOCK_PAGETABLE;
		} else if (bitmap_is_set(span->physblock_bitmap, i)) {
			block->type = PHYSBLOCK_PHYSBLOCK;
		} else {
			// We default to movable.
			block->type = PHYSBLOCK_KERNEL | PHYSBLOCK_MOVABLE;
		}

		block->refcount = 1;

	next:
		spinlock_release(&block->lock);
	}
}

void early_init_mem_map(void)
{
	// For each range of available physical memory, allocate a physblock per
	// page and map from KERNEL_MEM_MAP_ADDRESS.

	// Note we ZERO intiialise all these as doing this causes allocations
	// (both the physblock pages and the pagetables pointing at them) so
	// trying to mark pages as pagetables/not is pointless. We will assign
	// what these pages are in the second pass.
	for (int i = 0; i < (int)alloc_state->num_spans; i++) {
		struct early_page_alloc_span *span = &alloc_state->spans[i];

		alloc_map_physblock(span->start, span->num_pages);
	}

	// Second pass, initialising the physblocks.
	for (int i = 0; i < (int)alloc_state->num_spans; i++) {
		init_physblock_span(&alloc_state->spans[i]);
	}
}

void early_init_phys_alloc_state(void)
{
	if (sizeof(struct phys_alloc_state) +
		    sizeof(struct phys_alloc_span) * alloc_state->num_spans >
	    PAGE_SIZE)
		early_panic(
			"Too many spans to allocate phys alloc state in 1 page");

	physaddr_t pa = early_page_alloc_zero();
	phys_alloc_init_state(phys_to_virt_ptr(pa), alloc_state);
}

void early_mem_init(void)
{
	struct early_boot_info *info = early_get_boot_info();

	drop_direct0();
	early_sort_e820(info);
	early_merge_e820(info);
	early_normalise_e820(info);
	early_set_total_ram(info);
	early_scratch_alloc_init();
	early_page_alloc_init(info);
	early_remap_page_tables(info);
	early_init_phys_alloc_state();
	early_init_mem_map();
}
