/*-
* FIPS 180-2 SHA-224/256/384/512 implementation
* Last update: 05/23/2005
* Issue date:  04/30/2005
*
* Copyright (C) 2005 Olivier Gay <olivier.gay@a3.epfl.ch>
* Copyright (C) 2009 Leo Franchi <lfranchi@kde.org>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the project nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#ifndef _SHA2_H
#define _SHA2_H

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

#endif /* !_SHA2_H */

