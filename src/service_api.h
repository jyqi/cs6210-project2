/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
\*************************************************************************/
#ifndef SERVICE_API_H
#define SERVICE_API_H

#include <string.h>
#include <stdlib.h>  /* Used for malloc */
#include <sys/mman.h> /* mmap and munmap */
#include <sys/stat.h>   /* Defines mode constants */
#include <fcntl.h>  /* Defines file descriptor constants: O_ */
#include <mqueue.h>   /* Required to implement POSIX message queues */
#include <semaphore.h> /* Needed for semaphore */
#include <unistd.h> /* Needed for write function */

#include "errors.h"

#define SHM_NAME "/shm_caesar"
#define REG_MQ_NAME "/mq_registration"
#define SEM_MUTEX_NAME "/sem_mutex"
#define BUFSIZE 256

/* Used for color in Linux terminal output */
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"

struct shared_memory {
  char message[BUFSIZE+1];
  int shift;
};

void service_rotate(const char client_q_name[], char message[], int shift);

void service_register(const char client_q_name[], int priority_arg);

void service_deregister(const char client_q_name[]);

#endif
