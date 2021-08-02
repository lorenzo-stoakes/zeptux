#pragma once

// CPUID flags:
#define X86_LONGMODE_FLAG (1UL << 29)

// Global Descriptor Table (GDT) entry flags:
#define X86_GDTE_FLAG_4K_GRANULARITY   (1UL << 3)
#define X86_GDTE_FLAG_1B_GRANULARITY   (0   << 3)
// This one is weird - in a 64-bit code segment this must be unset, but we also
// set this for our data segment in long mode.
#define X86_GDTE_FLAG_32BIT_PROTECTED  (1UL << 2)
#define X86_GDTE_FLAG_16BIT_PROTECTED  (0   << 2)
#define X86_GDTE_FLAG_64BIT_CODE       (1UL << 1)
#define X86_GDTE_ACCESS_PRESENT        (1UL << 7)
#define X86_GDTE_ACCESS_RING0          (0)
#define X86_GDTE_ACCESS_RING1          (1UL << 5)
#define X86_GDTE_ACCESS_RING2          (2UL << 5)
#define X86_GDTE_ACCESS_RING3          (3UL << 5)
#define X86_GDTE_ACCESS_SYSTEM         (0   << 5)
#define X86_GDTE_ACCESS_NONSYS         (1UL << 4)
#define X86_GDTE_ACCESS_EXEC           (1UL << 3)
#define X86_GDTE_ACCESS_DATA_GROW_UP   (1UL << 2)
#define X86_GDTE_ACCESS_DATA_GROW_DOWN (0   << 2)
#define X86_GDTE_ACCESS_RING_CONFORM   (1UL << 2)
#define X86_GDTE_ACCESS_RING_NOCONFORM (0   << 2)
#define X86_GDTE_ACCESS_RW             (1UL << 1)
#define X86_GDTE_ACCESS_ACCESSED       (1UL << 0)

// Control register flags
#define X86_CR0_PROTECTED_MODE         (1UL << 0)
#define X86_CR0_PAGED_MODE             (1UL << 31)
#define X86_CR4_PAE                    (1UL << 5)

#define X86_MFR_EFER (0xc0000080UL)

#define X86_MFR_EFER_LME (1UL << 8) // Enable long mode.
