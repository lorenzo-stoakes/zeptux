#!/usr/bin/python3

"""
Patches the specified binary file with a uint32 at the specified offset.
"""

from sys import argv

if len(argv) < 3:
	print(f'usage: {argv[0]} [filename] [offset] [uint32 to write]')
	exit(1)

[ _, filename, offset, val ] = argv

with open(filename, 'r+b') as f:
	f.seek(int(offset))
	bs = int(val).to_bytes(4, byteorder='little')
	f.write(bs)
