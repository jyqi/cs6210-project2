/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // Defines _exit function
#include <stdlib.h>
#include "errors.h"
#include "ename.c.inc"

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

typedef enum { FALSE, TRUE } Boolean;

#ifdef __GNUC__
__attribute__ ((__noreturn__))
#endif

static void
terminate(Boolean useExit3)
{
    char *s;

    s = getenv("EF_DUMPCORE");

    if (s != NULL && *s != '\0')
        abort();
    else if (useExit3)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
}


static void
output_error(Boolean useErr, int err, Boolean flushStdout,
        const char *format, va_list ap)
{
#define BUF_SIZE 500
    char buf[BUF_SIZE], userMsg[BUF_SIZE], errText[BUF_SIZE];

    vsnprintf(userMsg, BUF_SIZE, format, ap);

    if (useErr) {
        snprintf(errText, BUF_SIZE, " [%s %s]",
                (err > 0 && err <= MAX_ENAME) ?
                ename[err] : "?UNKNOWN?", strerror(err));
    } else {
        snprintf(errText, BUF_SIZE, ":");
    }

    snprintf(buf, BUF_SIZE, "ERROR%s %s\n", errText, userMsg);

    if (flushStdout)
        fflush(stdout);
    fputs(buf, stderr);
    fflush(stderr); /* In case stderr is not line-buffered */
}

void
error_exit(const char *format, ...)
{
  va_list arg_list; /* va_list is a type defined in stdarg.h */

  va_start(arg_list, format);
  output_error(TRUE, errno, TRUE, format, arg_list);
  va_end(arg_list);
  terminate(TRUE);
}

/* program_type is SERVICE or CLIENT */
void
usage_error(const char *program_name, const int program_type)
{

    fflush(stdout); // Flush any pending stdout

    switch (program_type) {
        case SERVICE:
            fprintf(stderr, "Caesar Service v0.1\n");
            fprintf(stderr, "Usage: ./%s [-h] [-d] \n", program_name);
            fprintf(stderr, "     -h    Prints this usage information\n");
            fprintf(stderr, "     -d    Start service in daemon mode\n");
            fprintf(stderr, "     --version Prints the program version.\n");
            fprintf(stderr, "     --help Prints this usage information (same as -h).\n");
            break;
        case CLIENT:
            fprintf(stderr, "Caesar Client v0.1\n");
            fprintf(stderr, "Usage: ./%s [-h] [-m message] [-s shift] [-q name]\n", program_name);
            fprintf(stderr, "     -h    Prints this usage information\n");
            fprintf(stderr, "     -m    the message (plaintext or encoded)\n");
            fprintf(stderr, "     -s    Amount to shift (positive or negative)\n");
            fprintf(stderr, "     -q    the base name of the client queue\n");
            fprintf(stderr, "     --version Prints the program version.\n");
            fprintf(stderr, "     --help Prints this usage information (same as -h).\n");
            fprintf(stderr, "NOTE: message, shift, and queue arguments must be used together!\n");
            fprintf(stderr, "NOTE: -q arguments cannot be longer than 239 characters!!  This is because the max size of a message queue name is 255 and we will append a send/receive identifer to it.\n");
            break;
        default:
            fprintf(stderr, "CLI usage error.  Try ./%s --help\n", program_name);
    }
    exit(EXIT_FAILURE);
}
