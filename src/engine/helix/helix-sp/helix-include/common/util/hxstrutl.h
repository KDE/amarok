
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

#ifndef _HXSTRUTL_H_
#define _HXSTRUTL_H_

#include "hlxclib/string.h" /* for strxxx functions */
#include "hlxclib/stdlib.h" /* for atoi64() and itoa() functionallity */

#include "safestring.h"

#if !defined(_VXWORKS)
#ifdef _UNIX
#include <strings.h>
#include <ctype.h>
#endif
#endif
#ifdef _MACINTOSH
#include <ctype.h>
#endif

#include "hxresult.h"

#if defined (_MACINTOSH) 

#define isascii isprint

inline const char *AnsiNext(const char* pcPtr) { return(	 pcPtr + 1 ); }
inline const char *AnsiPrev(const char * /* pcStart */, const char* pcPtr) { return (pcPtr - 1 ); }

int CopyP2CString(ConstStr255Param inSource, char* outDest, int inDestLength);
void CopyC2PString(const char* inSource, Str255 outDest);
char WINToMacCharacter( char inWINChar );
// these functions are used to convert Windows extended chars (used in non-English Roman languages)
// to Mac extended chars & vice-versa
void StripWinChars( char* pChars);
void StripMacChars( char* pChars);

inline void pstrcpy(Str255 dst, ConstStr255Param src) { BlockMoveData(src, dst, 1+src[0]); }

#ifndef _CARBON
inline void PStrCopy(StringPtr dest, ConstStr255Param src) { BlockMoveData(src, dest, 1+src[0]); }
inline void p2cstrcpy(char *dst, ConstStr255Param src) { CopyP2CString(src, dst, 255); }
inline void c2pstrcpy(Str255 dst, const char * src) { CopyC2PString(src, dst); }
#endif

#endif /* _MACINTOSH */

#define CR		(CHAR) '\r'
#define LF              (CHAR) '\n'
#define CRLF            "\r\n"

#ifdef _WIN32
    #define LINEBREAK	    "\015\012"
    #define LINEBREAK_LEN   2
#else
    #define LINEBREAK	    "\012"
    #define LINEBREAK_LEN   1
#endif /* _WIN32 */

#define LINE_BUFFER_SIZE	4096
#define MAX_BYTES_PER_COOKIE	4096
#define MAX_NUMBER_OF_COOKIES	300
#define MAX_COOKIES_PER_SERVER	20

/*
According to C99 7.4/1:
---------------
The header <ctype.h> declares several functions useful for
classifying and mapping characters. In all cases the argument is an
int, the value of which shall be representable as an unsigned char or
shall equal the value of the macro EOF. If the argument has any other
value, the behavior is undefined.
---------------
Typecast the value to an (unsigned char) before passing it to isspace() to ensure that
if the value is a signed char it doesn't get bit extended on certain (VC) compilers.
*/
#define IS_SPACE(x)	(isspace((unsigned char) x))

#ifdef __cplusplus
void	    StrAllocCopy(char*& pDest, const char* pSrc);
#else
void	    StrAllocCopy(char** pDest, const char* pSrc);
#endif
char*	    StripLine(char* pLine);

#include "hxtypes.h"
#include "hxcom.h"
typedef _INTERFACE IHXValues IHXValues;
HX_RESULT   SaveStringToHeader(IHXValues* /* IN OUT */    pHeader, 
			       const char*  /* IN */	    pszKey, 
			       const char* /* IN */	    pszValue);

char* StrStrCaseInsensitive(const char* str1, const char* str2);
char* StrNStr(const char* str1, const char* str2, size_t depth1, size_t depth2);
char *StrNChr(const char *str, int c, size_t depth);
char *StrNRChr(const char *str, int c, size_t depth);
size_t StrNSpn(const char *str1, const char *str2, size_t depth1, size_t depth2);
size_t StrNCSpn(const char *str1, const char *str2, size_t depth1, size_t depth2);

char* StrToUpper(char *pString);

#if defined( _SYMBIAN)
#define NEW_FAST_TEMP_STR(NAME, EstimatedBiggestSize, LenNeeded)	\
    char*   NAME = new char[(LenNeeded)];		        

#define DELETE_FAST_TEMP_STR(NAME)					\
    delete[] NAME;							

#else
/* XXXSMP We can use alloca() on platforms that support it for more speed! */
#define NEW_FAST_TEMP_STR(NAME, EstimatedBiggestSize, LenNeeded)	\
    char    __##NAME##__StaticVersion[EstimatedBiggestSize];		\
    char*   NAME;							\
    UINT32  ulNeeded##NAME##Len = (LenNeeded);				\
        								\
    if (ulNeeded##NAME##Len <= EstimatedBiggestSize)			\
    {   								\
        NAME = __##NAME##__StaticVersion;				\
    }       								\
    else								\
    {									\
        NAME = new char[ulNeeded##NAME##Len];				\
    }

#define DELETE_FAST_TEMP_STR(NAME)					\
    if (NAME != __##NAME##__StaticVersion)				\
    { 									\
        delete[] NAME;							\
    }
#endif /* defined(_SYMBIAN) */

#endif /* _HXSTRUTL_H_ */
