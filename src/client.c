/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
\*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* Needed for getopt cli parsing */
#include <string.h> /* Needed for strcmp */
#include "service_api.h"
#include "errors.h"

#define VERSION "0.1"
#define BUFSIZE 256

int
main(int argc, char **argv)
{
    int opt;
    char message[BUFSIZE];
    int shift = 0;
    int priority = -1;

    /* Names of Shared Memory, Message Queues, and Semaphores */
    char client_q_name[BUFSIZE];
    client_q_name[0] = '\0';
    memset(message, 0, sizeof message);

    if (argc < 2)
        usage_error(argv[0], CLIENT);

    if (!strcmp(argv[1],"--help")) {
      usage_error(argv[0], CLIENT);
    } else if (!strcmp(argv[1],"--version")) {
      printf("Caesar Service v%s\n",VERSION);
      exit(EXIT_SUCCESS);
    }

    while ((opt = getopt(argc, argv, "hm:s:q:p:")) != -1) {
        switch (opt) {
            case 'h':
                usage_error(argv[0], CLIENT);
                break;
            case 'm': /* provide the message to encode/decode */
                snprintf( message, BUFSIZE, "%s", optarg );
                break;
            case 's':
                shift = atoi(optarg);
                break;
            case 'q':
                if(strlen(optarg) > 239) { // up to 16 characters will be concatenated to this string, max queue name size is 255
                    usage_error(argv[0], CLIENT);
                }
                snprintf( client_q_name, BUFSIZE, "%s", optarg );
                break;
            case 'p':
                priority = atoi(optarg);
                if(priority > 10 || priority < 0) { // Linux supports up to 32,768, but I'm going to set max at 10
                    usage_error(argv[0], CLIENT);
                }
                break;
            default:
                usage_error(argv[0], CLIENT);
        }
    }

    if (message[0] == 0 || shift == 0 || client_q_name[0] == '\0')
        usage_error(argv[0], CLIENT);

    if (priority == -1) // no priority argument given to program
        priority = 0; // default priority of 0

    service_register(client_q_name, priority);
    service_rotate(client_q_name, message, shift);
    service_deregister(client_q_name);

    return EXIT_SUCCESS;
}
