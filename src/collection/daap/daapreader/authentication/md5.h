/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef __MD5_H__
#define __MD5_H__

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#include <inttypes.h>
#else
typedef unsigned int uint32_t;
#endif

#include "portability.h"

typedef struct {
    uint32_t buf[4];
    uint32_t bits[2];
    unsigned char in[64];
    int apple_ver;
} MD5_CTX;

void OpenDaap_MD5Init(MD5_CTX *, int apple_ver);
void OpenDaap_MD5Update(MD5_CTX *, const unsigned char *, unsigned int);
void OpenDaap_MD5Final(MD5_CTX *, unsigned char digest[16]);

#endif /* __WINE_MD5_H__ */
