
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

#ifndef HLXSYS_STRING_H
#define HLXSYS_STRING_H

#if defined(_OPENWAVE)
#include "platform/openwave/hx_op_stdc.h"
#else
#include <string.h>
#endif /* !_OPENWAVE */

#ifdef _SYMBIAN
//on symbian we have stuff scattered all about.
# include <stdlib.h>
# include <ctype.h>
#endif
#if !defined(_VXWORKS)
#ifdef _UNIX
#include <strings.h>
#endif /* _UNIX */
#endif /* !defined(_VXWORKS) */

/* If we are on Windows and are compiling
 * a .c file and are using Visual C++,
 * then use __inline instead of inline.
 */
#if (defined(_WINDOWS) || defined(_OPENWAVE)) && \
    !defined(__cplusplus) && defined(_MSC_VER)
#define HLX_INLINE __inline
#else
#define HLX_INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char * __helix_strrev(char * str);
void __helix_strlwr(char *s);
void __helix_strupr(char *s);

const char* __helix_strnchr(const char* sc, const char c, size_t n);
const char* __helix_strnstr(const char* sc, const char* str, size_t n);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#define strnchr __helix_strnchr
#define strnstr __helix_strnstr

#ifdef _WINDOWS
#ifdef _WINCE
 int strcasecmp(const char* str1, const char* str2);
 #else
HLX_INLINE int
strcasecmp(const char* str1, const char* str2)
{
    return _stricmp(str1, str2);
}
#endif //_WINCE
HLX_INLINE int
strncasecmp(const char* str1, const char* str2, int len)
{
    return _strnicmp(str1, str2, (size_t) len);
}

#if defined(WIN32_PLATFORM_PSPC) 
#define strrev __helix_strrev
#define stricmp strcasecmp
#define strcmpi strcasecmp
#define strnicmp strncasecmp
#define strlwr __helix_strlwr
#define strupr __helix_strupr
#endif /* defined(WIN32_PLATFORM_PSPC) */

#endif /* _WINDOWS */


#if defined(_SYMBIAN)
unsigned long __helix_strtoul(const char*s, char**end, int base);
#define strtoul __helix_strtoul
#define strrev __helix_strrev
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define strlwr __helix_strlwr
#define strupr __helix_strupr

#endif /* _SYMBIAN */

#if defined(_OPENWAVE)
#define strcmpi stricmp
#define strrev __helix_strrev
#define strlwr __helix_strlwr
#undef stricmp
#undef strnicmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif /* _OPENWAVE_ARMULATOR */

#if defined (_MACINTOSH) 

#ifdef _MAC_MACHO

#define strlwr __helix_strlwr
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define strrev __helix_strrev

#else

int strnicmp(const char *first, const char *last, size_t count);
int stricmp(const char *first, const char *last);
char * strrev(char * str);

#endif

#define strcmpi stricmp

#ifndef _MAC_MACHO

HLX_INLINE int
strcasecmp(const char* str1, const char* str2)
{
    return stricmp(str1, str2);
}

#endif


#endif /* _MACINTOSH */

#if defined (_UNIX) && !defined (__QNXNTO__)

/* strcasecmp, strncasecmp are defined in strings.h */
#define stricmp  strcasecmp
#define strcmpi  strcasecmp
#define strnicmp strncasecmp

// Convert integer to string

// reverse a string in place

#define strrev __helix_strrev
#define strlwr __helix_strlwr
#define strupr __helix_strupr

#endif /* _UNIX */


#if defined (_MACINTOSH) || defined (_UNIX)
#define _tcsspn strspn
#define _tcscspn strcspn
#define _tcsrchr strchr
#define _tcsstr strstr
#endif

#ifdef _VXWORKS
extern "C" {
int strncasecmp(const char *first, const char *last, size_t count);
int strcasecmp(const char *first, const char *last);
}
#endif /* _VXWORKS */


#if defined(_WINDOWS) && !defined(__cplusplus) && defined(_MSC_VER)
#include "hlxclib/stdlib.h"     // malloc()

#define NEW_STRING_BUFFER new_string_buffer
#define HLX_ALLOC(x)      ((char*) malloc((x)))
#define HLX_DEALLOC(x)    free((x))

#elif defined(__cplusplus)
#define NEW_STRING_BUFFER new_string
#define HLX_ALLOC(x)      (new char[(x)])
#define HLX_DEALLOC(x)    delete [] (x)
#endif /* #elif defined(__cplusplus) */

#ifdef HLX_ALLOC
HLX_INLINE char*
NEW_STRING_BUFFER(const void* mem, int len)
{
    char* str = HLX_ALLOC(len+1);
    if (str)
    {
        memcpy((void*)str, mem, len); /* Flawfinder: ignore */
        str[len] = '\0';
    }
    return str;
}

HLX_INLINE char*
new_string(const char* str)
{
    char* pTmp = HLX_ALLOC(strlen(str)+1);

    return pTmp ? strcpy(pTmp, str) : NULL; /* Flawfinder: ignore */
}

HLX_INLINE char*
new_path_string(const char* str)
{
    char* pnew = HLX_ALLOC(strlen(str) + 1);
    const char* psrc = str;
    char* pdst = pnew;
    if (!pnew) return NULL;

    while (*psrc)
    {
        if (*psrc == '/' || *psrc == '\\')
        {
#if defined _WIN32 || defined(_SYMBIAN)
            *pdst = '\\';
#elif defined _UNIX || defined _OPENWAVE
            *pdst = '/';
#elif defined __MWERKS__
            *pdst = ':';
#else
	    *pdst = *psrc;
#endif
        }
        else
        {
            *pdst = *psrc;
        }
        psrc++;
        pdst++;
    }
    *pdst = '\0';
    return pnew;
}

HLX_INLINE void
delete_string(char* str)
{
    if (str)
    {
	HLX_DEALLOC(str);
    }
}
#endif /* HLX_ALLOC */

#endif /* HLXSYS_STRING_H */
