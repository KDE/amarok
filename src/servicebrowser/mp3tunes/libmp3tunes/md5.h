/*
 * This single function is taken from the Message-Digest algorithm
 * written by Ulrich Drepper in 1995, and edited by Gray Watson.
 * It was obtained from http://256.com/sources/md5/
 * Follows is the original comment header.
 *
 * Declaration of functions and data types used for MD5 sum computing
 * library functions.  Copyright (C) 1995, 1996 Free Software
 * Foundation, Inc.  NOTE: The canonical source of this file is
 * maintained with the GNU C Library.  Bugs can be reported to
 * bug-glibc@prep.ai.mit.edu.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * $Id: md5.h,v 1.4 2000/03/09 04:06:41 gray Exp $
 */
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
void	md5_sig_to_string(void *signature, char *str, const int str_len)
{
  unsigned char	*sig_p;
  char		*str_p, *max_p;
  unsigned int	high, low;
  
  str_p = str;
  max_p = str + str_len;
  
  for (sig_p = (unsigned char *)signature;
       sig_p < (unsigned char *)signature + MD5_SIZE;
       sig_p++) {
    high = *sig_p / 16;
    low = *sig_p % 16;
    /* account for 2 chars */
    if (str_p + 1 >= max_p) {
      break;
    }
    *str_p++ = HEX_STRING[high];
    *str_p++ = HEX_STRING[low];
  }
  /* account for 2 chars */
  if (str_p < max_p) {
    *str_p++ = '\0';
  }
}
