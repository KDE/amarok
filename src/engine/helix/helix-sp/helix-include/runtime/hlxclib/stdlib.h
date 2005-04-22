
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

#ifndef HLXSYS_STDLIB_H
#define HLXSYS_STDLIB_H

#include "hxtypes.h"

#if defined(_OPENWAVE)
// XXXSAB Include compiler <stdlib.h> so we can modify it???
#ifdef _OPENWAVE_SIMULATOR
#ifndef _WIN32
#define STDLIB_UNDEF_WIN32
#define _WIN32
#endif /* _WIN32 */
#endif /* _OPENWAVE_SIMULATOR */

#include <stdlib.h>
#undef itoa                     // just in case

#ifdef STDLIB_UNDEF_WIN32
#undef _WIN32
#undef STDLIB_UNDEF_WIN32
#endif /* STDLIB_UNDEF_WIN32 */

// XXXSAB Define malloc()/free() wrappers for Openwave in here???
#else
#include <stdlib.h>
#endif

char* __helix_itoa(int val, char *str, int radix);
char* __helix_i64toa(INT64 val, char *str, int radix);
INT64 __helix_atoi64(char* str);
void* __helix_bsearch( const void *key, const void *base, size_t num, 
		       size_t width, 
		       int (  *compare ) ( const void *elem1, 
					   const void *elem2 ) );
int __helix_remove(const char* pPath);
int __helix_putenv(const char* pStr);
char* __helix_getenv(const char* pName);

#if defined(_WINDOWS) && !defined(_OPENWAVE)

#if !defined(WIN32_PLATFORM_PSPC)
_inline char*
i64toa(INT64 val, char* str, int radix)
{
    return _i64toa(val, str, radix);
}

#else /* !defined(WIN32_PLATFORM_PSPC) */

_inline
int remove(const char* pPath)
{
    return __helix_remove(pPath);
}

_inline
char* getenv(const char* pName)
{
    return __helix_getenv(pName);
}

#define i64toa __helix_i64toa
#define itoa __helix_itoa

_inline
void* bsearch( const void *key, const void *base, size_t num, 
	       size_t width, 
	       int ( *compare ) ( const void *elem1, 
				  const void *elem2 ) )
{
    return __helix_bsearch(key, base, num, width, compare);
}
#endif /* !defined(WIN32_PLATFORM_PSPC) */

_inline INT64
atoi64(const char* str)
{
    return _atoi64(str);
}
#endif /* _WINDOWS */

#if defined (_MACINTOSH) 

#define itoa __helix_itoa
#define i64toa __helix_i64toa
#define atoi64 __helix_atoi64

#endif /* _MACINTOSH */

#if defined (_UNIX) && !defined (__QNXNTO__)

// Convert integer to string

#define itoa __helix_itoa
#define i64toa __helix_i64toa
#define atoi64 __helix_atoi64

#endif /* _UNIX */

#if defined(_SYMBIAN)

#define itoa __helix_itoa
#define i64toa __helix_i64toa
#define atoi64 __helix_atoi64
#define putenv __helix_putenv

#endif

#if defined(_OPENWAVE)

#define itoa(v,s,r) __helix_itoa((v),(s),(r))
#define i64toa(v,s,r) __helix_i64toa((v),(s),(r))
#define atoi64(s) __helix_atoi64((s))
#define putenv __helix_putenv

__inline int remove(const char* pPath)
{
    return __helix_remove(pPath);
}

#endif // _OPENWAVE

#endif /* HLXSYS_STDLIB_H */
