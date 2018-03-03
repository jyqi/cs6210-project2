/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#include "caesar.h"

/**
* reverse() - reverses a character array
* @a: The character array being reversed.
* @sz: The size of the array.
*
* Reverses a character array i.e. 'array' becomes 'yarra'.
*
*/
void reverse(char a[], int sz) {
    int i, j;
    for (i = 0, j = sz; i < j; i++, j--) {
        int tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

/**
* rotate() - rotates an array by amt
* @array: The character array being reversed.
* @size: The size of the array.
* @amt: amount of rotations
*
* Rotates a character array i.e. a single rotation of 'Hello' becomes 'oHell'.
*
*/
void rotate(char array[], int size, int shift) {
    /* Credits to Jon Bentley for the elegant rotation algorithm */
    if (shift < 0)
        shift = size + shift;
    reverse(array, size-shift-1);
    reverse(array+size-shift, shift-1);
    reverse(array, size-1);
}

/**
* getindex() - gets the index of a given letter in the rotated alphabet
* @c: The character you are looking for.
* @alphabet: a pointer to the alphabet you are using (array of uppercase or lowercase letters).
*
* For example, if you want the index of C in the alphabet, this function
* will return 2 (0-indexed).
*
* Return: the index of the character in the rotated alphabet array OR -1 if not found
*
*/
int getindex(char c, char *alphabet)
{
    unsigned int i = 0;
    // if c is an uppercase or lowercase letter
    if ((c >=65 && c <= 90) || (c >= 97 && c <= 122)) {
        // find the index of c in the rotated alphabet
        for(i = 0; i < strlen(alphabet); i++) {
            if(c == alphabet[i]) {
                // return the index of c in the rotated alphabet
                return i;
            }
        }
    }
    return -1;
}

/**
* rotx() - rotate a given message (caesar encode or decode)
* @message: a pointer to a character array with our message
* @shift: the number of rotations to shift (positive or negative value)
*
* Takes a character array and rotates it by a given shift value (this will encode or decode caesar ciphers)
*
*/
void rotx(char message[], int shift)
{
    char uppercase[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char lowercase[27] = "abcdefghijklmnopqrstuvwxyz";
    char u_tmp[27];
    char l_tmp[27];
    unsigned int j = 0;
    int index = 0;
    char msg_tmp[257];
    snprintf( msg_tmp, strlen(message)+1, "%s", message );

    strcpy(u_tmp,uppercase);
    strcpy(l_tmp,lowercase);
    rotate(u_tmp, strlen(u_tmp), shift);
    rotate(l_tmp, strlen(l_tmp), shift);
    for (j = 0; j < strlen(msg_tmp); j++) {
        index = getindex(msg_tmp[j],u_tmp);
        if (index >= 0) {
            message[j] = uppercase[index];
        } else {
            index = getindex(msg_tmp[j],l_tmp);
            if (index >= 0) {
                message[j] = lowercase[index];
            } else { // character at this index remains the same
                message[j] = message[j];
            }
        }
    }
}
