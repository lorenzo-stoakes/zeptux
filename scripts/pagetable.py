#!/usr/bin/python3

from sys import argv

data_mask = (1 << 12) - 1
pagedir_mask = (1 << 9) - 1

if len(argv) < 2:
    print(f'usage: {argv[0]} 0x[address]')
    exit(1)

[ _, address_str ] = argv

address = int(address_str, 16)

# 12 bits of offset into data page.
data_offset = address & data_mask
address >>= 12

# 9 bits of PTD offset
ptd_offset = address & pagedir_mask
address >>= 9

# 9 bits of PMD offset
pmd_offset = address & pagedir_mask
address >>= 9

# 9 bits of PUD offset
pud_offset = address & pagedir_mask
address >>= 9

# 9 bits of PGD offset
pgd_offset = address & pagedir_mask

print(f'{address_str}: PGD={pgd_offset}, PUD={pud_offset}, PMD={pmd_offset}, PTD={ptd_offset}, data={data_offset}')
