/**
 * Very simple file system (VSFS) data structures & Layout.
 *
 * This part borrows code heavily from xv6.
 */


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "vsfs.h"
#include "block.h"
#include "file.h"
#include "sysfile.h"
#include "exec.h"

#include "../common/debug.h"
#include "../common/string.h"
#include "../common/bitmap.h"
#include "../common/spinlock.h"
#include "../common/parklock.h"

#include "../memory/kheap.h"

#include "../process/process.h"
#include "../process/scheduler.h"


/** In-memory runtime copy of the FS meta-data structures. */
superblock_t superblock;

bitmap_t inode_bitmap;
bitmap_t data_bitmap;


/**
 * Allocates a free file descriptor of the caller process. Returns -1
 * if no free fd in this process.
 */
static int8_t
_alloc_process_fd(file_t *file)
{
    process_t *proc = running_proc();

    for (int8_t fd = 0; fd < MAX_FILES_PER_PROC; ++fd) {
        if (proc->files[fd] == NULL) {
            proc->files[fd] = file;
            return fd;
        }
    }

    return -1;
}

/** Find out what's the `file_t` for given FD for current process. */
static file_t *
_find_process_file(int8_t fd)
{
    process_t *proc = running_proc();

    if (fd < 0 || fd >= MAX_FILES_PER_PROC || proc->files[fd] == NULL)
        return NULL;
    return proc->files[fd];
}


/**
 * Look for a filename in a directory. Returns a got inode on success, or
 * NULL if not found. If found, sets *ENTRY_OFFSET to byte offset of the
 * entry.
 * Must be called with lock on DIR_INODE held.
 */
static mem_inode_t *
_dir_find(mem_inode_t *dir_inode, char *filename, uint32_t *entry_offset)
{
    assert(dir_inode->d_inode.type == INODE_TYPE_DIR);

    /** Search for the filename in this directory. */
    dentry_t dentry;
    for (size_t offset = 0;
         offset < dir_inode->d_inode.size;
         offset += sizeof(dentry_t)) {
        if (inode_read(dir_inode, (char *) &dentry, offset,
                       sizeof(dentry_t)) != sizeof(dentry_t)) {
            warn("dir_find: failed to read at offset %u", offset);
            return NULL;
        }
        if (dentry.valid == 0)
            continue;

        /** If matches, get the inode. */
        if (strncmp(dentry.filename, filename, MAX_FILENAME) == 0) {
            if (entry_offset != NULL)
                *entry_offset = offset;
            return inode_get(dentry.inumber);
        }
    }

    return NULL;
}

/** 
 * Add a new directory entry.
 * Must be called with lock on DIR_INODE held.
 */
static bool
_dir_add(mem_inode_t *dir_inode, char *filename, uint32_t inumber)
{
    /** The name must not be present. */
    mem_inode_t *file_inode = _dir_find(dir_inode, filename, NULL);
    if (file_inode != NULL) {
        warn("dir_add: file '%s' already exists", filename);
        inode_put(file_inode);
        return false;
    }

    /** Look for an emtpy directory entry. */
    dentry_t dentry;
    uint32_t offset;
    for (offset = 0;
         offset < dir_inode->d_inode.size;
         offset += sizeof(dentry_t)) {
        if (inode_read(dir_inode, (char *) &dentry, offset,
                       sizeof(dentry_t)) != sizeof(dentry_t)) {
            warn("dir_add: failed to read at offset %u", offset);
            return false;
        }
        if (dentry.valid == 0)
            break;
    }

    /** Add into this empty slot. */
    memset(&dentry, 0, sizeof(dentry_t));
    strncpy(dentry.filename, filename, MAX_FILENAME);
    dentry.inumber = inumber;
    dentry.valid = 1;
    if (inode_write(dir_inode, (char *) &dentry, offset,
                    sizeof(dentry_t)) != sizeof(dentry_t)) {
        warn("dir_add: failed to write at offset %u", offset);
        return false;
    }

    return true;
}

/**
 * Returns true if the directory is empty. Since directory size grows
 * in Hux and never gets recycled until removed, need to loop over all
 * allocated dentry slots to check if all are now unused.
 * Must be called with lock on DIR_INODE held.
 */
static bool
_dir_empty(mem_inode_t *dir_inode)
{
    dentry_t dentry;
    for (size_t offset = 2 * sizeof(dentry_t);      /** Skip '.' and '..' */
         offset < dir_inode->d_inode.size;
         offset += sizeof(dentry_t)) {
        if (inode_read(dir_inode, (char *) &dentry, offset,
                       sizeof(dentry_t)) != sizeof(dentry_t)) {
            warn("dir_empty: failed to read at offset %u", offset);
            return false;
        }
        if (dentry.valid != 0)
            return false;
    }

    return true;
}

/**
 * Get filename for child inode of INUMBER.
 * Must be called with lock on DIR_INODE held.
 */
static size_t
_dir_filename(mem_inode_t *dir_inode, uint32_t inumber,
              char *buf, size_t limit)
{
    dentry_t dentry;
    for (size_t offset = 2 * sizeof(dentry_t);      /** Skip '.' and '..' */
         offset < dir_inode->d_inode.size;
         offset += sizeof(dentry_t)) {
        if (inode_read(dir_inode, (char *) &dentry, offset,
                       sizeof(dentry_t)) != sizeof(dentry_t)) {
            warn("dir_filename: failed to read at offset %u", offset);
            return limit;
        }
        if (dentry.valid == 0)
            continue;

        if (dentry.inumber == inumber) {
            size_t len = limit - 1;
            if (len < strlen(dentry.filename))
                return limit;
            else if (len > strlen(dentry.filename))
                len = strlen(dentry.filename);
            strncpy(buf, dentry.filename, len);
            return len;
        }
    }

    warn("dir_filename: child inumber %u not found", inumber);
    return limit;
}


/**
 * Copy the next path element into ELEM, and returns the rest path with
 * leading slashes removed.
 * E.g., _parse_elem("aaa///bb/c") sets ELEM to "aaa" and returns a pointer
 * to "bb/c". If there are no elements, return NULL.
 */
static char *
_parse_elem(char *path, char *elem)
{
    while (*path == '/')
        path++;
    if (*path == '\0')
        return NULL;

    char *elem_start = path;
    while (*path != '/' && *path != '\0')
        path++;
    size_t elem_len = path - elem_start;
    if (elem_len > MAX_FILENAME - 1)
        elem_len = MAX_FILENAME - 1;
    memcpy(elem, elem_start, elem_len);
    elem[elem_len] = '\0';

    while (*path == '/')
        path++;
    return path;
}

/**
 * Search the directory tree and get the inode for a path name.
 * Returns NULL if not found.
 *
 * If STOP_AT_PARENT is set, returns the inode for the parent
 * directory and writes the final path element into FILENAME,
 * which must be at least MAX_FILENAME in size.
 */
static mem_inode_t *
_path_to_inode(char *path, bool stop_at_parent, char *filename)
{
    mem_inode_t *inode;

    /** Starting point. */
    if (*path == '/')
        inode = inode_get(ROOT_INUMBER);
    else {
        inode = running_proc()->cwd;
        inode_ref(inode);
    }
    if (inode == NULL) {
        warn("path_lookup: failed to get starting point of %s", path);
        return NULL;
    }

    /** For each path element, go into that directory. */
    do {
        path = _parse_elem(path, filename);
        if (path == NULL)
            break;

        inode_lock(inode);

        if (inode->d_inode.type != INODE_TYPE_DIR) {
            inode_unlock(inode);
            inode_put(inode);
            return NULL;
        }

        if (stop_at_parent && *path == '\0') {
            inode_unlock(inode);
            return inode;     /** Stopping one-level early. */
        }

        mem_inode_t *next = _dir_find(inode, filename, NULL);
        if (next == NULL) {
            inode_unlock(inode);
            inode_put(inode);
            return NULL;
        }

        inode_unlock(inode);
        inode_put(inode);
        inode = next;
    } while (path != NULL);

    if (stop_at_parent) {
        inode_put(inode);   /** Path does not contain parent. */
        return NULL;
    }

    return inode; 
}

/** Wrappers for path lookup. */
static mem_inode_t *
_path_lookup(char *path)
{
    char filename[MAX_FILENAME];
    return _path_to_inode(path, false, filename);
}

static mem_inode_t *
_path_lookup_parent(char *path, char *filename)
{
    return _path_to_inode(path, true, filename);
}


/**
 * Open a file for the caller process. Returns the file descriptor (>= 0)
 * on success, and -1 on failure.
 */
int8_t
filesys_open(char *path, uint32_t mode)
{
    mem_inode_t *inode = _path_lookup(path);
    if (inode == NULL)
        return -1;

    inode_lock(inode);

    if (inode->d_inode.type == INODE_TYPE_DIR && mode != OPEN_RD) {
        inode_unlock(inode);
        inode_put(inode);
        return -1;
    }

    file_t *file = file_get();
    if (file == NULL) {
        warn("open: failed to allocate open file structure, reached max?");
        inode_unlock(inode);    // Maybe use goto.
        inode_put(inode);
        return -1;
    }

    int8_t fd = _alloc_process_fd(file);
    if (fd < 0) {
        warn("open: failed to allocate file descriptor, reached max?");
        file_put(file);
        inode_unlock(inode);    // Maybe use goto.
        inode_put(inode);
        return -1;
    }

    inode_unlock(inode);

    file->inode = inode;
    file->readable = (mode & OPEN_RD) != 0;
    file->writable = (mode & OPEN_WR) != 0;
    file->offset = 0;

    return fd;
}

/** Closes an open file, actually closing if reference count drops to 0. */
bool
filesys_close(int8_t fd)
{
    file_t *file = _find_process_file(fd);
    if (file == NULL) {
        warn("close: cannot find file for fd %d", fd);
        return false;
    }

    running_proc()->files[fd] = NULL;
    file_put(file);
    return true;
}


/**
 * Create a file or directory at the given path name. Returns true on
 * success and false on failures.
 */
bool
filesys_create(char *path, uint32_t mode)
{
    char filename[MAX_FILENAME];
    mem_inode_t *parent_inode = _path_lookup_parent(path, filename);
    if (parent_inode == NULL) {
        warn("create: cannot find parent directory of '%s'", path);
        return false;
    }

    inode_lock(parent_inode);

    mem_inode_t *file_inode = _dir_find(parent_inode, filename, NULL);
    if (file_inode != NULL) {
        warn("create: file '%s' already exists", path);
        inode_unlock(parent_inode);
        inode_put(parent_inode);
        inode_put(file_inode);
        return false;
    }

    uint32_t type = (mode & CREATE_FILE) ? INODE_TYPE_FILE : INODE_TYPE_DIR;
    file_inode = inode_alloc(type);
    if (file_inode == NULL) {
        warn("create: failed to allocate inode on disk, out of space?");
        return false;
    }

    inode_lock(file_inode);

    /** Create '.' and '..' entries for new directory. */
    if (type == INODE_TYPE_DIR) {
        if (!_dir_add(file_inode, ".", file_inode->inumber)
            || !_dir_add(file_inode, "..", parent_inode->inumber)) {
            warn("create: failed to create '.' or '..' entries");
            inode_free(file_inode);
            return false;
        }
    }

    /** Put file into parent directory. */
    if (!_dir_add(parent_inode, filename, file_inode->inumber)) {
        warn("create: failed to put '%s' into its parent directory", path);
        inode_free(file_inode);
        return false;
    }

    inode_unlock(parent_inode);
    inode_put(parent_inode);

    inode_unlock(file_inode);
    inode_put(file_inode);

    return true;
}

/**
 * Remove a file or directory from the file system. If is removing a
 * directory, the directory must be empty.
 */
bool
filesys_remove(char *path)
{
    char filename[MAX_FILENAME];
    mem_inode_t *parent_inode = _path_lookup_parent(path, filename);
    if (parent_inode == NULL) {
        warn("remove: cannot find parent directory of '%s'", path);
        return false;
    }

    inode_lock(parent_inode);

    /** Cannot remove '.' or '..'. */
    if (strncmp(filename, ".", MAX_FILENAME) == 0
        || strncmp(filename, "..", MAX_FILENAME) == 0) {
        warn("remove: cannot remove '.' or '..' entries");
        inode_unlock(parent_inode);     // Maybe use goto.
        inode_put(parent_inode);
        return false;
    }

    uint32_t offset;
    mem_inode_t *file_inode = _dir_find(parent_inode, filename, &offset);
    if (file_inode == NULL) {
        warn("remove: cannot find file '%s'", path);
        inode_unlock(parent_inode);     // Maybe use goto.
        inode_put(parent_inode);
        return false;
    }

    inode_lock(file_inode);

    /** Cannot remove a non-empty directory. */
    if (file_inode->d_inode.type == INODE_TYPE_DIR
        && !_dir_empty(file_inode)) {
        warn("remove: cannot remove non-empty directory '%s'", path);
        inode_unlock(file_inode);       // Maybe use goto.
        inode_put(file_inode);
        inode_unlock(parent_inode);     // Maybe use goto.
        inode_put(parent_inode);
        return false;
    }

    /** Write zeros into the corresponding entry in parent directory. */
    dentry_t dentry;
    memset(&dentry, 0, sizeof(dentry_t));
    if (inode_write(parent_inode, (char *) &dentry, offset,
                    sizeof(dentry_t)) != sizeof(dentry_t)) {
        warn("remove: failed to write at offset %u", offset);
        inode_unlock(file_inode);       // Maybe use goto.
        inode_put(file_inode);
        inode_unlock(parent_inode);     // Maybe use goto.
        inode_put(parent_inode);
        return false;
    }

    inode_unlock(parent_inode);
    inode_put(parent_inode);

    /** Erase its metadata on disk. */
    inode_free(file_inode);

    inode_unlock(file_inode);
    inode_put(file_inode);

    return true;
}


/** Read from current offset of file into user buffer. */
int32_t
filesys_read(int8_t fd, char *dst, size_t len)
{
    file_t *file = _find_process_file(fd);
    if (file == NULL) {
        warn("read: cannot find file for fd %d", fd);
        return -1;
    }

    if (!file->readable) {
        warn("read: file for fd %d is not readable", fd);
        return -1;
    }

    inode_lock(file->inode);
    size_t bytes_read = inode_read(file->inode, dst, file->offset, len);
    if (bytes_read > 0)         /** Update file offset. */
        file->offset += bytes_read;
    inode_unlock(file->inode);

    return bytes_read;
}

/** Write from user buffer into current offset of file. */
int32_t
filesys_write(int8_t fd, char *src, size_t len)
{
    file_t *file = _find_process_file(fd);
    if (file == NULL) {
        warn("read: cannot find file for fd %d", fd);
        return -1;
    }

    if (!file->writable) {
        warn("write: file for fd %d is not writable", fd);
        return -1;
    }

    inode_lock(file->inode);
    size_t bytes_written = inode_write(file->inode, src, file->offset, len);
    if (bytes_written > 0)      /** Update file offset. */
        file->offset += bytes_written;
    inode_unlock(file->inode);

    return bytes_written;
}


/** Change the current working directory (cwd) of caller process. */
bool
filesys_chdir(char *path)
{
    mem_inode_t *inode = _path_lookup(path);
    if (inode == NULL) {
        warn("chdir: target path '%s' does not exist", path);
        return false;
    }

    inode_lock(inode);
    if (inode->d_inode.type != INODE_TYPE_DIR) {
        warn("chdir: target path '%s' is not a directory", path);
        inode_unlock(inode);
        inode_put(inode);
        return false;
    }
    inode_unlock(inode);

    /** Put the old cwd and keep the new one. */
    process_t *proc = running_proc();
    inode_put(proc->cwd);
    proc->cwd = inode;

    return true;
}

/** Get an absolute string path of current working directory. */
static size_t
_recurse_abs_path(mem_inode_t *inode, char *buf, size_t limit)
{
    if (inode->inumber == ROOT_INUMBER) {
        buf[0] = '/';
        return 1;
    }

    inode_lock(inode);

    /** Check the parent directory. */
    mem_inode_t *parent_inode = _dir_find(inode, "..", NULL);
    if (parent_inode == NULL) {
        warn("abs_path: failed to get parent inode of %u", inode->inumber);
        inode_unlock(inode);
        return limit;
    }

    inode_unlock(inode);

    /** If parent is root, stop recursion.. */
    if (parent_inode->inumber == ROOT_INUMBER) {
        buf[0] = '/';

        inode_lock(parent_inode);
        size_t written = _dir_filename(parent_inode, inode->inumber,
                                       &buf[1], limit - 1);
        inode_unlock(parent_inode);
        inode_put(parent_inode);

        return 1 + written;
    }

    size_t curr = _recurse_abs_path(parent_inode, buf, limit);
    if (curr >= limit - 1)
        return limit;

    inode_lock(parent_inode);
    size_t written = _dir_filename(parent_inode, inode->inumber,
                                   &buf[curr], limit - curr);
    inode_unlock(parent_inode);
    inode_put(parent_inode);

    return curr + written;
}

bool
filesys_getcwd(char *buf, size_t limit)
{
    mem_inode_t *inode = running_proc()->cwd;
    inode_ref(inode);

    size_t written = _recurse_abs_path(inode, buf, limit);
    if (written >= limit)
        return false;
    else
        buf[limit - 1] = '\0';

    inode_put(inode);
    return true;
}


/** Wrapper over `exec_program()`. */
bool
filesys_exec(char *path, char **argv)
{
    mem_inode_t *inode = _path_lookup(path);
    if (inode == NULL) {
        warn("exec: failed to lookup path '%s'", path);
        return false;
    }

    char *filename = &path[strlen(path) - 1];
    while (*filename != '/' && filename != path)
        filename--;
    return exec_program(inode, filename, argv);
}


/** Get metadata information about an open file. */
bool
filesys_fstat(int8_t fd, file_stat_t *stat)
{
    file_t *file = _find_process_file(fd);
    if (file == NULL) {
        warn("fstat: cannot find file for fd %d", fd);
        return false;
    }

    file_stat(file, stat);
    return true;
}


/** Seek to absolute file offset. */
bool
filesys_seek(int8_t fd, size_t offset)
{
    file_t *file = _find_process_file(fd);
    if (file == NULL) {
        warn("seek: cannot find file for fd %d", fd);
        return false;
    }

    inode_lock(file->inode);
    size_t filesize = file->inode->d_inode.size;
    inode_unlock(file->inode);

    if (offset > filesize) {
        warn("seek: offset %lu beyond filesize %lu", offset, filesize);
        return false;
    }

    file->offset = offset;
    return true;
}


/** Flush the in-memory modified bitmap block to disk. */
bool
inode_bitmap_update(uint32_t slot_no)
{
    return block_write((char *) &(inode_bitmap.bits[slot_no]),
                       superblock.inode_bitmap_start * BLOCK_SIZE + slot_no, 1);
}

bool
data_bitmap_update(uint32_t slot_no)
{
    return block_write((char *) &(data_bitmap.bits[slot_no]),
                       superblock.data_bitmap_start * BLOCK_SIZE + slot_no, 1);
}


/**
 * Initialize the file system by reading out the image from the
 * IDE disk and parse according to VSFS layout.
 */
void
filesys_init(void)
{
    /** Block 0 must be the superblock. */
    if (!block_read_at_boot((char *) &superblock, 0, sizeof(superblock_t)))
        error("filesys_init: failed to read superblock from disk");

    /**
     * Currently the VSFS layout is hard-defined, so just do asserts here
     * to ensure that the mkfs script followed the expected layout. In real
     * systems, mkfs should have the flexibility to generate FS image as
     * long as the layout is valid, and we read out the actual layout here.
     */
    assert(superblock.fs_blocks == 262144);
    assert(superblock.inode_bitmap_start == 1);
    assert(superblock.inode_bitmap_blocks == 6);
    assert(superblock.data_bitmap_start == 7);
    assert(superblock.data_bitmap_blocks == 32);
    assert(superblock.inode_start == 39);
    assert(superblock.inode_blocks == 6105);
    assert(superblock.data_start == 6144);
    assert(superblock.data_blocks == 256000);

    /** Read in the two bitmaps into memory. */
    uint32_t num_inodes = superblock.inode_blocks * (BLOCK_SIZE / INODE_SIZE);
    uint8_t *inode_bits = (uint8_t *) kalloc(num_inodes / 8);
    bitmap_init(&inode_bitmap, inode_bits, num_inodes);
    if (!block_read_at_boot((char *) inode_bitmap.bits,
                            superblock.inode_bitmap_start * BLOCK_SIZE,
                            num_inodes / 8)) {
        error("filesys_init: failed to read inode bitmap from disk");
    }

    uint32_t num_dblocks = superblock.data_blocks;
    uint8_t *data_bits = (uint8_t *) kalloc(num_dblocks / 8);
    bitmap_init(&data_bitmap, data_bits, num_dblocks);
    if (!block_read_at_boot((char *) data_bitmap.bits,
                            superblock.data_bitmap_start * BLOCK_SIZE,
                            num_dblocks / 8)) {
        error("filesys_init: failed to read data bitmap from disk");
    }

    /** Fill open file table and inode table with empty slots. */
    for (size_t i = 0; i < MAX_OPEN_FILES; ++i) {
        ftable[i].ref_cnt = 0;      /** Indicates UNUSED. */
        ftable[i].readable = false;
        ftable[i].writable = false;
        ftable[i].inode = NULL;
        ftable[i].offset = 0;
    }
    spinlock_init(&ftable_lock, "ftable_lock");

    for (size_t i = 0; i < MAX_MEM_INODES; ++i) {
        icache[i].ref_cnt = 0;      /** Indicates UNUSED. */
        icache[i].inumber = 0;
        parklock_init(&(icache[i].lock), "inode's parklock");
    }
    spinlock_init(&icache_lock, "icache_lock");
}
