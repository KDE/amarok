
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 *
 */

#ifndef _SAFESTRING_H_
#define _SAFESTRING_H_

#include "hxtypes.h"            /* UINT32 */
#include "hlxclib/string.h"     /* for strxxx functions */

#ifdef __cplusplus		
extern "C" {
#endif  /* __cplusplus */

char* SafeStrCpy(char* pDestStr, const char* pSourceStr, UINT32 ulBufferSize);
char* SafeStrCat(char* pDestStr, const char* pSourceStr, UINT32 ulBufferSize);

#ifdef _OPENWAVE_ARMULATOR
#define SafeSprintf op_snprintf
#else
int SafeSprintf(char* pBuffer, UINT32 ulBufferSize, const char* pFormatStr, ...);
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _SAFESTRING_H_ */
