/****************************************************************************************
 * Copyright (c) 1995,1996 Free Software Foundation Inc. <info@fsf.org>                 *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef _LIBMP3TUNES_MD5_H
#define _LIBMP3TUNES_MD5_H

#include <stdlib.h>
#include <string.h>

#define MD5_SIZE    16
#define HEX_STRING  "0123456789abcdef"  /* to convert to hex */
/*#include <sys/types.h>*/

/*
 * md5_sig_to_string
 *
 * DESCRIPTION:
 *
 * Convert a MD5 signature in a 16 byte buffer into a hexadecimal string
 * representation.
 *
 * RETURNS:
 *
 * None.
 *
 * ARGUMENTS:
 *
 * signature - a 16 byte buffer that contains the MD5 signature.
 *
 * str - a string of charactes which should be at least 33 bytes long (2
 * characters per MD5 byte and 1 for the \0).
 *
 * str_len - the length of the string.
 */
void md5_sig_to_string(void *signature, char *str, const int str_len);

/*
 * md5_calc_file_signature
 *
 * DESCRIPTION:
 *
 * Calculates MD5 signature of the specified file contents and returns
 * hexadecimal representation of the signature.
 *
 * RETURNS:
 *
 * Hexadecimal representation of the signature of NULL if it could not be
 * calculated. The returned pointer should be freed with the free()
 * function when it is no longer needed.
 *
 * ARGUMENTS:
 *
 * filename - a path to the file which MD5 signature should be calculated.
 */
char* md5_calc_file_signature(const char *filename);

#endif
