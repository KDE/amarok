/****************************************************************************************
 * Copyright (c) 2005 Olivier Gay <olivier.gay@a3.epfl.ch>                              *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef SHA256_H
#define SHA256_H

#define SHA256_DIGEST_SIZE (256 / 8)
#define SHA512_DIGEST_SIZE (512 / 8)

#define SHA256_BLOCK_SIZE  ( 512 / 8)

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct {
        unsigned int tot_len;
        unsigned int len;
        unsigned char block[2 * SHA256_BLOCK_SIZE];
        uint32_t h[8];
    } sha256_ctx;
    
    void sha256_init(sha256_ctx * ctx);
    void sha256_update(sha256_ctx *ctx, unsigned char *message, unsigned int len);
    void sha256_final(sha256_ctx *ctx, unsigned char *digest);
    void sha256(unsigned char *message, unsigned int len, unsigned char *digest);
    
#ifdef __cplusplus
}
#endif

#endif /* !SHA256_H */

