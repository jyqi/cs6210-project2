/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#ifndef POSIXSHM_H
#define POSIXSHM_H

#include <stddef.h> /* Needed for size_t type */
#include <stdlib.h> /* Prototypes of commonly used library functions
                       EXIT_SUCCESS and EXIT_FAILURE constants */
#include <sys/mman.h> /* mmap and munmap */
#include <sys/stat.h> /* for mode constants */
#include <fcntl.h>  /* Needed for file descriptor flags O_RDONLY, O_RDWR, etc */
#include <stdlib.h> /* Prototypes of commonly used library functions
                       EXIT_SUCCESS and EXIT_FAILURE constants */
#include <stdio.h> /* Standard I/O functions */
#include <unistd.h> /* Needed for write function */
#include <errno.h>  /* Needed to check errno values */
#include <string.h> /* Needed for memcpy and strlen functions */

#include "errors.h"

extern void * create_shm(const char *name, size_t size);

extern void * write_shm(const char *name, const char *data);

extern void * read_shm(const char *name);

extern void remove_shm(const char *name);

#endif
