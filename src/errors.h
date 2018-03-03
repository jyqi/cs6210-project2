/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#ifndef ERRORS_H
#define ERRORS_H

#include <stdarg.h> /* provides type va_list, and macros va_start, va_end used with errors.c */
#include <stdio.h>

#define SERVICE 1
#define CLIENT 0

#ifdef __GNUC__

/* This macro stops 'gcc -Wall' complaining that "control reaches end of
 * non-void function" if we use the following functions to terminate main()
 * or some other non-void function. */

#define NORETURN __attribute__ ((__noreturn__))
#else
#define NORETURN
#endif

void error_exit(const char *format, ...);
/* program_type is SERVICE or CLIENT */
void usage_error(const char *program_name, const int program_type);

#endif
