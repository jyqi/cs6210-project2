/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#include "posixshm.h"

/**
* create_shm() - creates a POSIX shared memory object.
* @name: The desired name of the shared memory object.
* @size: The size in bytes of the shared memory object.
*
* This function is just a wrapper around shm_open and mmap to include some
* additional error checking as well as to set some default flags and perms.
*
* Returns: the shared memory pointer
*
*/
extern void * create_shm(const char *name, size_t size)
{
  int flags, fd;
  mode_t mode;
  void *addr;

  flags = O_CREAT | O_RDWR;
  mode = S_IRUSR | S_IWUSR;

  fd = shm_open(name, flags, mode);
  if (fd == -1)
    error_exit("shm_open");

  if (ftruncate(fd, size) == -1)
    error_exit("ftruncate");

  addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED)
    error_exit("mmap");

  if (close(fd) == -1)
    error_exit("close (create_shm)");

  return addr;
}

/**
* write_shm() - Copies data into a POSIX shared memory object
* @name: The name of the existing memory object.
* @data: The data (a string) you wish to copy to shared memory.
*
* This function takes all the steps necessary to write to shared memory.
* This includes function calls to shm_open, ftruncate, mmap, and memcpy.
*
*/
extern void * write_shm(const char *name, const char *data)
{
  int flags, fd;
  size_t len;                /* Size of shared memory object */
  char *addr;

  flags = O_RDWR;
  fd = shm_open(name, flags, 0);  /* Opens existing memory object */
  if (errno == ENOENT)
    error_exit("shm doesn't exist (write_shm)");

  if (fd == -1)
    error_exit("shm_open (write_shm)");

  len = strlen(data);
  if (ftruncate(fd, len) == -1)   /* Resize object to hold string */
    error_exit("ftruncate (write_shm)");
  printf("write_shm: Resized to %ld bytes\n", (long) len);

  addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED)
    error_exit("mmap failed (write_shm)");

  if (close(fd) == -1)
    error_exit("close (write_shm)");

  printf("write_shm: copying %ld bytes\n", (long) len);
  memcpy(addr, data, len);             /* Copy string to shared memory */

  return addr;
}

/**
* read_shm() - creates a POSIX shared memory object.
* @fd: description
*
* Longer description
*
* Return: a file descriptor to the shared memory object
*/
extern void * read_shm(const char *name)
{
  int fd;
  char *addr;
  struct stat sb;

  fd = shm_open(name, O_RDONLY, 0);    /* Open existing object */
  if (fd == -1)
    error_exit("shm_open (read_shm)");

  /* Use shared memory object size as length argument for mmap()
   * and as number of bytes to write() */

  if (fstat(fd, &sb) == -1)
    error_exit("fstat (read_shm)");

  addr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED)
    error_exit("mmap (read_shm)");

  if (close(fd) == -1)
    error_exit("close (read_shm)");

  write(STDOUT_FILENO, addr, sb.st_size);
  printf("\n");

  return addr;
}

/**
* remove_shm() - creates a POSIX shared memory object.
* @fd: description
*
* Just a wrapper around shm_unlink that checks for failure
* If shm_unlink fails, calls error_exit
*
* Return: a file descriptor to the shared memory object
*/
extern void remove_shm(const char *name)
{
  if (shm_unlink(name) == -1)
    error_exit("shm_unlink (remove_shm)");
}
