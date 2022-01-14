#!/usr/bin/python3

#
# The mkfs script which formats a VSFS file system initial image
# according to the VSFS layout definiton at `src/filesys/vsfs.h`.
#
# Copies the user program executables under 'user/' (except `init`) in.
#


import os
import sys
from pathlib import Path, PurePath
from pprint import PrettyPrinter

class MyPrinter(PrettyPrinter):
    """
    Automatically shorten excessively long bytearrays.
    Credit: https://stackoverflow.com/questions/20514525/
            automatically-shorten-long-strings-when-dumping-with-pretty-print
    """
    def _format(self, object, *args, **kwargs):
        if isinstance(object, bytearray):
            if len(object) > 5:
                object = object[:5] + b'...'
        return PrettyPrinter._format(self, object, *args, **kwargs)


# These definitions should follow `src/filesys/vsfs.h` for now.
BLOCK_SIZE = 1024
INODE_SIZE = 128
DENTRY_SIZE = 128

INODES_PB = BLOCK_SIZE / INODE_SIZE
DENTRY_PB = BLOCK_SIZE / DENTRY_SIZE
UINT32_PB = BLOCK_SIZE / 4

FS_BLOCKS = 262144
INODE_BITMAP_START = 1
INODE_BITMAP_BLOCKS = 6
DATA_BITMAP_START = 7
DATA_BITMAP_BLOCKS = 32
INODE_START = 39
INODE_BLOCKS = 6105
DATA_START = 6144
DATA_BLOCKS = 256000

NUM_DIRECT    = 16
NUM_INDIRECT1 = 8
NUM_INDIRECT2 = 1

MAX_FILENAME = 120

INODE_TYPE_EMPTY = 0
INODE_TYPE_FILE  = 1
INODE_TYPE_DIR   = 2

ENDIANESS = 'little'    # x86 IA32 is little endian


# FS image bytearray.
img = bytearray(FS_BLOCKS * BLOCK_SIZE)     # Zero bytes by default

# Current inode slots & data blocks used.
curr_data_block = 0
curr_inumber = 1        # 0 is reserved for the root directory

# Initial directory tree as a dictionary, see `build_dtree()`.
dtree = dict()


def uint32_to_bytes(uint32):
    """
    Convert a number to uint32_t bytes.
    """
    return uint32.to_bytes(4, byteorder=ENDIANESS, signed=False)

def string_to_bytes(string):
    """
    Convert a string to NULL-terminated ASCII string.
    """
    return bytearray(string, encoding='ascii') + b'\x00'

def put_uint32(offset, uint32):
    """
    Put a uint32_t at given byte offset.
    """
    offset = int(offset)
    img[offset:offset+4] = uint32_to_bytes(uint32)

def put_bytearray(offset, barray):
    """
    Put a bytearray at given byte offset.
    """
    offset = int(offset)
    img[offset:offset+len(barray)] = barray


def gen_superblock():
    """
    Generate the superblock.
    """
    put_uint32(0 , FS_BLOCKS          )
    put_uint32(4 , INODE_BITMAP_START )
    put_uint32(8 , INODE_BITMAP_BLOCKS)
    put_uint32(12, DATA_BITMAP_START  )
    put_uint32(16, DATA_BITMAP_BLOCKS )
    put_uint32(20, INODE_START        )
    put_uint32(24, INODE_BLOCKS       )
    put_uint32(28, DATA_START         )
    put_uint32(32, DATA_BLOCKS        )


def add_data_block(block):
    """
    Put a block of bytes into a free data block.
    Returns the address of the block allocated.
    """
    global curr_data_block
    addr = (DATA_START + curr_data_block) * BLOCK_SIZE
    put_bytearray(addr, block)
    curr_data_block += 1
    return addr

def add_inode(file_size, data_blocks, inode_type, inumber):
    """
    Put an inode into given inode slot, also given file size and allocated
    data block addresses.
    """
    assert file_size > (len(data_blocks) - 1) * BLOCK_SIZE
    assert file_size <= len(data_blocks) * BLOCK_SIZE
    assert inode_type == INODE_TYPE_FILE or inode_type == INODE_TYPE_DIR

    addr = INODE_START * BLOCK_SIZE + inumber * INODE_SIZE
    
    put_uint32(addr,     inode_type)
    put_uint32(addr + 4, file_size)

    # Direct.
    idx = 0
    while idx < NUM_DIRECT:
        if idx >= len(data_blocks):
            break
        put_uint32(addr + 8 + idx*4, data_blocks[idx])
        idx += 1

    # Singly-indirect.
    while idx < (NUM_DIRECT + NUM_INDIRECT1*UINT32_PB):
        if idx >= len(data_blocks):
            break
        chopped_idx = idx - NUM_DIRECT
        indirect1_block = bytearray(BLOCK_SIZE)
        for i in range(0, BLOCK_SIZE, 4):
            if idx >= len(data_blocks):
                break
            indirect1_block[i:i+4] = uint32_to_bytes(data_blocks[idx])
            idx += 1
        indirect1_addr = add_data_block(indirect1_block)
        put_uint32(addr + 8 + NUM_DIRECT*4 + (chopped_idx//UINT32_PB)*4,
                   indirect1_addr)

    # Doubly-indirect.
    while idx < (NUM_DIRECT + NUM_INDIRECT1*UINT32_PB \
                 + NUM_INDIRECT2*(UINT32_PB**2)):
        if idx >= len(data_blocks):
            break
        chopped_idx = idx - NUM_DIRECT - NUM_INDIRECT1*UINT32_PB
        indirect1_block = bytearray(BLOCK_SIZE)
        for i in range(0, BLOCK_SIZE, 4):
            if idx >= len(data_blocks):
                break
            indirect2_block = bytearray(BLOCK_SIZE)
            for j in range(0, BLOCK_SIZE, 4):
                if idx >= len(data_blocks):
                    break
                indirect2_block[j:j+4] = uint32_to_bytes(data_blocks[idx])
                idx += 1
            indirect2_addr = add_data_block(indirect2_block)
            indirect1_block[i:i+4] = uint32_to_bytes(indirect2_addr)
        indirect1_addr = add_data_block(indirect1_block)
        put_uint32(addr + 8 + NUM_DIRECT*4 + NUM_INDIRECT1*4 \
                   + (chopped_idx//(UINT32_PB**2))*4,
                   indirect1_addr)

def add_file(content, my_inumber):
    """
    Add a regular file into the file system image.
    Returns the inumber assigned to this file.
    """
    data_blocks = []
    for block_beg in range(0, len(content), BLOCK_SIZE):
        block_end = min(block_beg + BLOCK_SIZE, len(content))
        data_blocks.append(add_data_block(content[block_beg:block_end]))

    return add_inode(len(content), data_blocks, INODE_TYPE_FILE,
                     my_inumber)

def add_dir(content, my_inumber, parent_inumber):
    """
    Add a directory into the file system image. All things inside this
    directory must have been added.
    Returns the inumber assigned to this directory.
    """
    names = list(content.keys())
    inumbers = list(map(lambda val: val[0], content.values()))

    # Add in default '.' ane '..' entries as the first two.
    names = [".", ".."] + names
    inumbers = [my_inumber, parent_inumber] + inumbers

    data_blocks = []
    idx = 0
    while idx < len(names):
        dir_block = bytearray(BLOCK_SIZE)
        for i in range(0, BLOCK_SIZE, DENTRY_SIZE):
            if idx >= len(names):
                break
            if len(names[idx]) > MAX_FILENAME:
                print("Error: file name '{}' exceeds max length".format(names[idx]))
                exit(1)
            if inumbers[idx] < 0 or inumbers[idx] >= INODE_BLOCKS * INODES_PB:
                print("Error: detected invalid inumber {}".format(inumbers[idx]))
                exit(1)
            dir_block[i:i+4]   = uint32_to_bytes(1)
            dir_block[i+4:i+8] = uint32_to_bytes(inumbers[idx])
            name_bytes = string_to_bytes(names[idx])
            dir_block[i+8:i+8+len(name_bytes)] = name_bytes
            idx += 1
        data_blocks.append(add_data_block(dir_block))

    return add_inode(len(names) * DENTRY_SIZE, data_blocks, INODE_TYPE_DIR,
                     my_inumber)


def gen_bitmaps():
    """
    Generate the inode bitmap and data bitmap according to the number of
    inode slots and data blocks used.
    """
    def prefix_bits(num):
        """
        Get a byte with prefix of NUM bits.
        """
        assert num >= 0 and num < 8
        prefix_dict = { 0: b'\x00',
                        1: b'\x80',
                        2: b'\xC0',
                        3: b'\xE0',
                        4: b'\xF0',
                        5: b'\xF8',
                        6: b'\xFC',
                        7: b'\xFE' }
        return prefix_dict[num]

    inode_bitmap = bytearray(0)
    for i in range(curr_inumber // 8):
        inode_bitmap.extend(b'\xFF')
    inode_bitmap.extend(prefix_bits(curr_inumber % 8))
    put_bytearray(INODE_BITMAP_START * BLOCK_SIZE, inode_bitmap)

    data_bitmap = bytearray(0)
    for i in range(curr_data_block // 8):
        data_bitmap.extend(b'\xFF')
    data_bitmap.extend(prefix_bits(curr_data_block % 8))
    put_bytearray(DATA_BITMAP_START * BLOCK_SIZE, data_bitmap)


def build_dtree(bins):
    """
    Build the directory tree out of what's under `user/`. The `dtree` is a
    dict of:
      string name -> 2-list [inumber, element]
    
    , where element could be:
      - Raw bytes for regular file
      - A `dict` for directory, which recurses on
    """
    def next_inumber():
        """
        Allocate the next available inumber.
        """
        global curr_inumber
        inumber = curr_inumber
        curr_inumber += 1
        return inumber

    for b in bins:
        bpath = Path(b)
        if not bpath.is_file():
            print("Error: user binary '{}' is not a regular file".format(b))
            exit(1)

        parts = PurePath(b).parts
        parents = parts[1:-1]
        binary = parts[-1]
        if parts[0] != "user":
            print("Error: user binray '{}' is not under 'user/'".format(b))
            exit(1)
        if not binary.endswith(".bin"):
            print("Error: user binray '{}' does not end with '.bin'".format(b))
            exit(1)
        binary = binary[:-4]

        curr_dir = dtree
        for d in parents:
            if d not in curr_dir:
                curr_dir[d] = [next_inumber(), dict()]
            curr_dir = curr_dir[d][1]

        with bpath.open(mode='br') as bfile:
            curr_dir[binary] = [next_inumber(), bytearray(bfile.read())]

def solidize_dtree():
    """
    Solidize the directory tree into the file system image bytearray.
    Runs a pre-order depth-first search (pre-DFS) over the tree, so that
    files inside a directory will be added after the directory.
    """
    def solidize_elem(elem, my_inumber, parent_inumber):
        """
        Recursively solidizes an element. If ELEM is a bytearray, it is
        a regular file. If ELEM is a dict, it is a directory and should
        recurse deeper if the directory contains things.
        """
        if isinstance(elem, bytearray):
            add_file(elem, my_inumber)
        elif isinstance(elem, dict):
            # First put the directory.
            add_dir(elem, my_inumber, parent_inumber)
            # Then the children so that a sub-directory would know '..'s inumber.
            for key, val in elem.items():
                solidize_elem(val[1], val[0], my_inumber)
        else:
            print("Error: unrecognized element type {}".format(type(elem)))
            exit(1)

    solidize_elem(dtree, 0, 0)      # "/"s '..' also points to "/"


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 {} output_name.img [user_binaries]".format(sys.argv[0]))
        exit(1)
    output_img = sys.argv[1]
    user_binaries = sys.argv[2:]

    if os.path.isfile(output_img):
        ans = input("WARN: image file '{}' exists, overwrite? (y/n) ".format(output_img))
        if ans != 'y':
            print(" Skipped!")
            exit(0)

    # Build the initial file system image.
    gen_superblock()
    build_dtree(user_binaries)
    solidize_dtree()
    gen_bitmaps()

    print("Initial FS image directory tree:")
    MyPrinter().pprint(dtree)

    # Flush the image bytes to output file.
    with open(output_img, mode='bw') as output_file:
        output_file.write(img)


if __name__ == '__main__':
    main()
