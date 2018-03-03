/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#include "posixmq.h"

/**
* create_mq() - creates a POSIX message queue for read/write
* @name: The desired name of the POSIX message queue.
*
* This function is just a wrapper around mq_open with additional error
* checking as well as to set some default flags and perms.
*
* Max Messages on queue are set to 50, and Max Message Size is set to 2048 bytes
*
* Return: the new message queue descriptor
*/
extern int create_mq(const char *name)
{
    int flags;
    mode_t perms;
    mqd_t mqd;
    struct mq_attr attr, *attrp;

    attrp = NULL;
    attr.mq_maxmsg = 50;
    attr.mq_msgsize = 2048;
    flags = O_CREAT | O_RDWR;
    perms = S_IRUSR | S_IWUSR;

    mqd = mq_open(name, flags, perms, attrp);
    if (mqd == (mqd_t) -1)
      error_exit("mq_open");

    return mqd;
}

/**
* close_mq() - closes a POSIX message queue and delete it
* @mqd: the message queue descriptor of type mqd_t
*
* This function is just a wrapper around mq_close and mq_unlink
*
* Return: EXIT_SUCCESS or fails with error message "mq_unlink"
*/
extern int close_mq(const char *name, mqd_t mqd)
{
    mq_close(mqd);
    if (mq_unlink(name) == -1)
      error_exit("mq_unlink");
    return EXIT_SUCCESS;
}
