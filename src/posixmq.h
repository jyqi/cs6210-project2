/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#ifndef POSIXMQ_H
#define POSIXMQ_H

#include <fcntl.h>  /* Defines file descriptor constants: O_ */
#include <sys/stat.h>   /* Defines mode constants */
#include <mqueue.h>   /* Required to implement POSIX message queues */
#include <stddef.h> /* Define NULL */
#include <stdlib.h> /* define EXIT_SUCCESS and EXIT_FAILURE */
#include <stdio.h> /* allow for printf */
#include "errors.h"

extern int create_mq(const char *name);

extern int write_mq(const char *name, const char *data);

extern int read_mq(const char *name);

extern int close_mq(const char *name, mqd_t mqd);

#endif
