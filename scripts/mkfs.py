#!/usr/bin/python3

#
# The mkfs script which builds a VSFS file system initial image
# according to the VSFS layout definiton at `src/filesys/vsfs.h`.
#
# Copies those user program executables in.
#


import os
import sys


# These definitions should follow `src/filesys/vsfs.h` for now.
BLOCK_SIZE = 1024
FS_BLOCKS = 262144
INODE_BITMAP_START = 1
INODE_BITMAP_BLOCKS = 6
DATA_BITMAP_START = 7
DATA_BITMAP_BLOCKS = 32
INODE_START = 39
INODE_BLOCKS = 6105
DATA_START = 6144
DATA_BLOCKS = 256000

ENDIANESS = 'little'    # x86 IA32 is little endian


# Put a uint32_t at given byte offset.
def put_byte(img, offset, number):
    img[offset:offset+4] = number.to_bytes(4, byteorder='little', signed=False)


# Generate the superblock.
def gen_superblock(img):
    put_byte(img, 0 , FS_BLOCKS          )
    put_byte(img, 4 , INODE_BITMAP_START )
    put_byte(img, 8 , INODE_BITMAP_BLOCKS)
    put_byte(img, 12, DATA_BITMAP_START  )
    put_byte(img, 16, DATA_BITMAP_BLOCKS )
    put_byte(img, 20, INODE_START        )
    put_byte(img, 24, INODE_BLOCKS       )
    put_byte(img, 28, DATA_START         )
    put_byte(img, 32, DATA_BLOCKS        )


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 {} output_name.img".format(sys.argv[0]))
        exit(1)
    output_img = sys.argv[1]

    if os.path.isfile(output_img):
        print("Output image file {} exists, stopping".format(output_img))
        exit(2)

    img = bytearray(FS_BLOCKS * BLOCK_SIZE)     # Zero bytes by default
    gen_superblock(img)
    # TODO

    with open(output_img, mode='bw') as output_file:
        output_file.write(img)


if __name__ == '__main__':
    main()
