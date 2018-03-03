/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#ifndef CAESAR_H
#define CAESAR_H

#include <stdio.h>
#include <stdlib.h> /* Prototypes of commonly used library functions
                       EXIT_SUCCESS and EXIT_FAILURE constants */
#include <string.h> /* Needed for memcpy, strlen, and strcpy functions */

void reverse(char a[], int sz);

void rotate(char array[], int size, int shift);

int getindex(char c, char *alphabet);

void rotx(char message[], int shift);

#endif
