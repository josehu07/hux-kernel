/**
 * Implementation of the `exec()` syscall on ELF-32 file.
 */


#ifndef EXEC_H
#define EXEC_H


#include <stdbool.h>

#include "file.h"


/** Maximum number of arguments allowed in `argv` list. */
#define MAX_EXEC_ARGS 32


bool exec_program(mem_inode_t *inode, char *filename, char **argv);


#endif
