
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

#if defined(_SYMBIAN)
# include <e32def.h>
# include <e32std.h>
# include <platform/symbian/symbiantypes.h> /* For our TInt64 impl */
#endif

#ifdef _VXWORKS
#include "types/vxTypesOld.h"
#include "vxWorks.h"
     /* md3 - added to override SENS macro. net/mbuf.h */
#    ifdef m_flags
#    undef m_flags
#    endif /* m_flags */
     /* md3 - added to override SENS macro, net/radix.h */
#    ifdef Free
#    undef Free
#    endif /* Free */
#endif

#ifdef _MACINTOSH
#pragma once
#endif

#ifndef _HXTYPES_H_
#define _HXTYPES_H_

// this is essential to make sure that new is not #define'd before the C++ version is included.
#if (defined(_MSC_VER) && defined(_DEBUG) && defined(__cplusplus) && !defined(WIN32_PLATFORM_PSPC) && !defined(_SYMBIAN) && !defined(_OPENWAVE))
#include <memory>
#endif

// disable the "debug info truncated at 255" warning.
#if defined _MSC_VER
#pragma warning (disable: 4786)
#endif

#if (defined(_MSC_VER) && (_MSC_VER > 1100) && defined(_BASETSD_H_))
#error For VC++ 6.0 or higher you must include hxtypes.h before other windows header files.
#endif

#if defined(_SYMBIAN)
typedef TInt8   INT8;
typedef TUint8  UINT8;
typedef TInt16  INT16;
typedef TUint16 UINT16;
typedef TInt32  INT32;
typedef TUint32 UINT32;
typedef TUint32 UINT; /* Its unclear, but UINT is suppose to be 32 bits. */
typedef TBool   BOOL;
#else
# ifndef _VXWORKS

#  if defined(QWS) && !defined(QT_CLEAN_NAMESPACE)
#error "You need to define QT_CLEAN_NAMESPACE when using Qt with Helix. If you don't you'll have conflicts with the Helix INT32, UINT32, and UINT definitions"
#  endif

   typedef char                    INT8;   /* signed 8 bit value */
   typedef unsigned char           UINT8;  /* unsigned 8 bit value */
   typedef short int               INT16;  /* signed 16 bit value */
   typedef unsigned short int      UINT16; /* unsigned 16 bit value */
#  if (defined _UNIX && defined _LONG_IS_64)
     typedef int                     INT32;  /* signed 32 bit value */
     typedef unsigned int            UINT32; /* unsigned 32 bit value */
     typedef unsigned int            UINT;
#  elif defined _VXWORKS
       typedef int                     INT32;  /* signed 32 bit value */
       typedef unsigned int            UINT32; /* unsigned 32 bit value */
       typedef unsigned int            UINT;
#  else
     typedef long int                INT32;  /* signed 32 bit value */
     typedef unsigned long int       UINT32; /* unsigned 32 bit value */
     typedef unsigned int            UINT;
#  endif /* (defined _UNIX && (defined _ALPHA || OSF1)) */

#  if (defined _UNIX && defined _IRIX)
#    ifdef __LONG_MAX__
#    undef __LONG_MAX__
#  endif
#  define __LONG_MAX__ 2147483647
# endif


#endif /* _VXWORKS */

#endif /* _SYMBIAN */


#if defined(HELIX_CONFIG_AVOID_BOOL)
    typedef int HXBOOL;
#   if defined(BOOL)
#       undef BOOL
#   endif
#else
#   if defined(BOOL)
        typedef BOOL HXBOOL;
#   else
        typedef int HXBOOL;
        typedef HXBOOL BOOL;
#   endif
#endif


#define ARE_BOOLS_EQUAL(a,b)	(((a) && (b)) || (!(a) && !(b)))

#ifndef HX_BITFIELD
typedef unsigned char HX_BITFIELD;
#endif

typedef INT32   LONG32;                 /* signed 32 bit value */
typedef UINT32  ULONG32;                /* unsigned 32 bit value */


#ifdef _LONG_IS_64
typedef long int                INT64;
typedef unsigned long int       UINT64;
#elif defined(_WINDOWS) || defined(_OPENWAVE_SIMULATOR)
typedef __int64			INT64;
typedef unsigned __int64	UINT64;
#elif defined(_SYMBIAN) && !defined (_SYMBIAN_81_)
typedef SymInt64                INT64;
typedef SymInt64                UINT64;
#else
typedef long long		INT64;
typedef unsigned long long	UINT64;
#endif /* _WINDOWS */


/* define the float and double type for all platforms */
#define HXFLOAT  float
#define HXDOUBLE double

typedef ULONG32 HX_MOFTAG;



/* Some platforms have native 64 bit int types, others don't
 * so, we provide these casting macros that have to be used
 * to cast 64-bit ints to smaller datatypes
 */
#if defined(_SYMBIAN) && !defined (_SYMBIAN_81_)
#define INT64_TO_ULONG32(a) ((ULONG32)((a).Low()))
#define INT64_TO_UINT32(a)  ((UINT32)((a).Low()))
#define INT64_TO_INT32(a)   ((INT32)((a).Low()))
#define INT64_TO_DOUBLE(a)  ((a).GetTReal())
#define INT64_TO_FLOAT(a)   ((a).GetTReal())
#define UINT32_TO_DOUBLE(a) (SymbianUINT32toDouble(a))
#else
#define INT64_TO_ULONG32(a) ((ULONG32)(a))
#define INT64_TO_UINT32(a)  ((UINT32) (a))
#define INT64_TO_INT32(a)   ((INT32)  (a))
#define INT64_TO_DOUBLE(a)  ((double) (a))
#define INT64_TO_FLOAT(a)   ((float) (a))
#define UINT32_TO_DOUBLE(a) ((double) (a))
#endif


#ifdef _MACINTOSH
    #ifdef powerc
    #define _MACPPC
    #else
    #define _MAC68K
    #endif
#endif

#if defined(_SYMBIAN)
#define PATH_MAX      KMaxPath
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#define LANGUAGE_CODE	"EN"

#ifdef _WIN16
#define MAX_PATH	260
#define PRODUCT_ID	"play16"
#define PLUS_PRODUCT_ID	"plus16"
#else
#define PRODUCT_ID	"play32"
#define PLUS_PRODUCT_ID	"plus32"
#endif

// $Private:
#define DEFAULT_CONN_TIMEOUT	20	    // in seconds
#define MAX_TIMESTAMP_GAP	0x2fffffff
#if !defined(MAX_UINT32)
#define MAX_UINT32		0xffffffff
#endif /* MAX_UINT32 */
#if defined(_MACINTOSH)
#define kLetInterruptsFinishBeforeQuittingGestalt 'RN$~'
#endif
// $EndPrivate.

#define MAX_DISPLAY_NAME        256
#define HX_INVALID_VALUE	(ULONG32)0xffffffff

#define HX_FREE(x) ((x) ? (free (x), (x) = 0) : 0)

#if defined(HELIX_CONFIG_NULL_DELETE_UNSAFE)

#define HX_DELETE(x) ((x) ? (delete (x), (x) = 0) : 0)
#define HX_VECTOR_DELETE(x) ((x) ? (delete [] (x), (x) = 0) : 0)

#else // defined(HELIX_CONFIG_NULL_DELETE_UNSAFE)

#define HX_DELETE(x) (delete (x), (x) = 0)
#define HX_VECTOR_DELETE(x) (delete [] (x), (x) = 0)

#endif // defined(HELIX_CONFIG_NULL_DELETE_UNSAFE)

#define RA_FILE_MAGIC_NUMBER    0x2E7261FDL /* RealAudio File Identifier */
#define RM_FILE_MAGIC_NUMBER    0x2E524D46L /* RealMedia File Identifier */
#define RIFF_FILE_MAGIC_NUMBER  0x52494646L /* RIFF (AVI etc.) File Identifier */

#ifndef _VXWORKS
typedef UINT8   UCHAR;                  /* unsigned 8 bit value */
#endif
typedef INT8    CHAR;                   /* signed 8 bit value */

typedef UINT8   BYTE;

typedef INT32   long32;
typedef UINT32  u_long32;

#ifndef _MACINTOSH
typedef INT8    Int8;
#endif
typedef UINT8   u_Int8;
typedef INT16   Int16;
typedef UINT16  u_Int16;
typedef INT32   Int32;
typedef UINT32  u_Int32;

/*
 * XXXGo
 * deprecated...now that we need UFIXED and FIXED, this name is confusing...
 * use the ones below.
 */
typedef ULONG32                 UFIXED32;           /* FIXED point value  */
#define FLOAT_TO_FIXED(x)   ((UFIXED32) ((x) * (1L << 16) + 0.5))
#define FIXED_TO_FLOAT(x)   ((float) ((((float)x)/ (float)(1L <<16))))

/*
 * float and fixed point value conversion
 */
#define HX_FLOAT_TO_UFIXED(x)   ((UFIXED32) ((x) * (1L << 16) + 0.5))
#define HX_UFIXED_TO_FLOAT(x)   ((float) ((((float)x)/ (float)(1L <<16))))

typedef LONG32                  FIXED32;          /* FIXED point value  */
#define HX_FLOAT_TO_FIXED(x)    ((FIXED32) ((x) * (1L << 16) + 0.5))
#define HX_FIXED_TO_FLOAT(x)    ((float) ((((float)x)/ (float)(1L <<16))))


/*
 * UFIXED32 is a 32 value where the upper 16 bits are the unsigned integer
 * portion of value, and the lower 16 bits are the fractional part of the
 * value
 */

typedef const char*             PCSTR;

/*
 *  FOURCC's are 32bit codes used in Tagged File formats like
 *  the RealMedia file format.
 */
#ifndef FOURCC
typedef UINT32                  FOURCC;
#endif

#ifndef HX_FOURCC
#define HX_FOURCC( ch0, ch1, ch2, ch3 )                                    \
                ( (UINT32)(UINT8)(ch0) | ( (UINT32)(UINT8)(ch1) << 8 ) |        \
                ( (UINT32)(UINT8)(ch2) << 16 ) | ( (UINT32)(UINT8)(ch3) << 24 ) )
#endif

typedef UINT16 PrefKey;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL                0
#else
#define NULL                ((void *)0)
#endif
#endif

#ifndef _WINDOWS	/* defined in windef.h on Windows platform */
#ifndef HIWORD
#define HIWORD(x) ((x) >> 16)
#endif
#ifndef LOWORD
#define LOWORD(x) ((x) & 0xffff)
#endif
#endif

#ifndef NOMINMAX
/* Always use macro versions of these! */
#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#endif

/* Should use capitalized macro versions of these, as the lowercase
		versions conflict with the STL spec */
//get these from sys/param.h to avoid tons of warnings about redefining them:
#ifdef _UNIX
#include <sys/param.h>
#endif
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#define HX_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define HX_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/*--------------------------------------------------------------------------
|   ZeroInit - initializes a block of memory with zeros
--------------------------------------------------------------------------*/
#define ZeroInit(pb)    memset((void *)pb,0,sizeof(*(pb)))

#ifndef __MACTYPES__
typedef unsigned char   Byte;
#endif

/*
/////////////////////////////////////////////////////////////////////////////
// HXEXPORT needed for RA.H and RAGUI.H, should be able to be defined
// and used in cross platform code...
/////////////////////////////////////////////////////////////////////////////
*/
#if defined(_WIN32) || defined(_WINDOWS)
#ifdef _WIN32
#define HXEXPORT            __declspec(dllexport) __stdcall
#define HXEXPORT_PTR        __stdcall *
#else /* Windows, but not 32 bit... */
#define HXEXPORT            _pascal __export
#define HXEXPORT_PTR        _pascal *
#define WAVE_FORMAT_PCM	    1
#define LPCTSTR				LPCSTR
#endif
#else /* Not Windows... */




#define HXEXPORT
#define HXEXPORT_PTR        *
#endif

#if defined(_WIN32) || defined(_WINDOWS) || defined (_MACINTOSH)|| defined (_UNIX)
typedef void (*RANOTIFYPROC)( void* );
#endif

#if defined(EXPORT_CLASSES) && defined(_WINDOWS)
#ifdef _WIN32
#define HXEXPORT_CLASS __declspec(dllexport)
#else
#define HXEXPORT_CLASS __export
#endif // _WIN32
#else
#define HXEXPORT_CLASS
#endif // EXPORT_CLASSES


/*
 *  STDMETHODCALLTYPE
 */
#ifndef STDMETHODCALLTYPE
#if defined(_WIN32) || defined(_MPPC_)
#ifdef _MPPC_
#define STDMETHODCALLTYPE       __cdecl
#else
#define STDMETHODCALLTYPE       __stdcall
#endif
#elif defined(_WIN16)
// XXXTW I made the change below on 5/18/98.  The __export was causing
//       conflicts with duplicate CHXBuffer methods in being linked into
//       rpupgrd and rpdestpn.  Also, the warning was "export imported".
//       This was fixed by removing the __export.  The __export is also
//       causing the same problem in pndebug methods.
//#define STDMETHODCALLTYPE       __export far _cdecl
#define STDMETHODCALLTYPE       far _cdecl
#else
#define STDMETHODCALLTYPE
#endif
#endif

/*
 *  STDMETHODVCALLTYPE  (V is for variable number of arguments)
 */
#ifndef STDMETHODVCALLTYPE
#if defined(_WINDOWS) || defined(_MPPC_)
#define STDMETHODVCALLTYPE      __cdecl
#else
#define STDMETHODVCALLTYPE
#endif
#endif

/*
 *  STDAPICALLTYPE
 */
#ifndef STDAPICALLTYPE
#if defined(_WIN32) || defined(_MPPC_)
#define STDAPICALLTYPE          __stdcall
#elif defined(_WIN16)
#define STDAPICALLTYPE          __export FAR PASCAL
#else
#define STDAPICALLTYPE
#endif
#endif

/*
 *  STDAPIVCALLTYPE (V is for variable number of arguments)
 */
#ifndef STDAPIVCALLTYPE
#if defined(_WINDOWS) || defined(_MPPC_)
#define STDAPIVCALLTYPE         __cdecl
#else
#define STDAPIVCALLTYPE
#endif
#endif

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      HX_GET_MAJOR_VERSION()
//
//  Purpose:
//
//      Returns the Major version portion of the encoded product version
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The major version number of the RealAudio application interface DLL
//
//
*/
#define HX_GET_MAJOR_VERSION(prodVer)   ((prodVer >> 28) & 0xF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      HX_GET_MINOR_VERSION()
//
//  Purpose:
//
//      Returns the minor version portion of the encoded product version
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The minor version number of the RealAudio application interface DLL
//
//
*/
#define HX_GET_MINOR_VERSION(prodVer)   ((prodVer >> 20) & 0xFF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      HX_GET_RELEASE_NUMBER()
//
//  Purpose:
//
//      Returns the release number portion of the encoded product version
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The release number of the RealAudio application interface DLL
//
//
*/
#define HX_GET_RELEASE_NUMBER(prodVer)  ((prodVer >> 12) & 0xFF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      HX_GET_BUILD_NUMBER()
//
//  Purpose:
//
//      Returns the build number portion of the encoded product version
//      of the RealAudio application interface DLL previously returned from
//      a call to RaGetProductVersion().
//
//  Parameters:
//
//      prodVer
//      The encoded product version of the RealAudio application interface
//      DLL previously returned from a call to RaGetProductVersion().
//
//  Return:
//
//      The build number of the RealAudio application interface DLL
//
//
*/
#define HX_GET_BUILD_NUMBER(prodVer)    (prodVer & 0xFFF)

/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      HX_ENCODE_PROD_VERSION()
//
//  Purpose:
//
//      Encodes a major version, minor version, release number, and build
//      number into a product version for testing against the product version
//      of the RealAudio application interface DLL returned from a call to
//      RaGetProductVersion().
//
//  Parameters:
//
//      major
//      The major version number to encode.
//
//      mimor
//      The minor version number to encode.
//
//      release
//      The release number to encode.
//
//      build
//      The build number to encode.
//
//  Return:
//
//      The encoded product version.
//
//  NOTES:
//
//      Macintosh DEVELOPERS especially, make sure when using the HX_ENCODE_PROD_VERSION
//      that you are passing a ULONG32 or equivalent for each of the parameters.
//      By default a number passed in as a constant is a short unless it requires more room,
//      so designate the constant as a long by appending a L to the end of it.
//      Example:
//          WORKS:
//              HX_ENCODE_VERSION(2L,1L,1L,0L);
//
//          DOES NOT WORK:
//              HX_ENCODE_VERSION(2,1,1,0);
//
*/

#define HX_ENCODE_PROD_VERSION(major,minor,release,build)   \
            ((ULONG32)((ULONG32)major << 28) | ((ULONG32)minor << 20) | \
            ((ULONG32)release << 12) | (ULONG32)build)

#define HX_ENCODE_ADD_PRIVATE_FIELD(ulversion,ulprivate) \
			((ULONG32)((ULONG32)(ulversion) & (UINT32)0xFFFFFF00) | (ULONG32)(ulprivate) )

#define HX_EXTRACT_PRIVATE_FIELD(ulversion)(ulversion & (UINT32)0xFF)

#define HX_EXTRACT_MAJOR_VERSION(ulversion) ((ulversion)>>28)
#define HX_EXTRACT_MINOR_VERSION(ulversion) (((ulversion)>>20) & (UINT32)0xFF)

#ifdef _AIX
    typedef int                 tv_sec_t;
    typedef int                 tv_usec_t;
#elif (defined _HPUX)
    typedef UINT32              tv_sec_t;
    typedef INT32               tv_usec_t;
#else
    typedef INT32               tv_sec_t;
    typedef INT32               tv_usec_t;
#endif /* _AIX */

#ifndef VOLATILE
#define VOLATILE volatile
#endif

typedef ULONG32 HXXRESOURCE;
typedef ULONG32 HXXHANDLE;
typedef ULONG32 HXXIMAGE;

// Macro which indicates that a particular variable is unused. Use this to
// avoid compiler warnings.
#define UNUSED(x)

/*
 * For VC++ 6.0 and higher we need to include this substitute header file
 * in place of the standard header file basetsd.h, since this standard
 * header file conflicts with our definitions.
 */
#if defined(_MSC_VER) && (_MSC_VER > 1100) && !defined(_SYMBIAN)
#include "hxbastsd.h"
#ifdef WIN32_PLATFORM_PSPC
#define _TYPES_H_
#endif
#endif

#ifdef _VXWORKS
/* throw in some defines for VXWORKS */
#define MAXPATHLEN 255


#endif

#ifdef _MACINTOSH
// xxxbobclark with CWPro 7, there is a code generation bug in the long long
// casting code. It can be avoided by first casting to an unsigned long long.
// This is supposedly fixed in the upcoming (as of this writing) CWPro 8, but
// for now we're setting up the casting using a macro to change it easily.
#define CAST_TO_INT64 (INT64)(UINT64)
#else
#define CAST_TO_INT64 (INT64)
#endif


#if defined _LONG_IS_64 && (defined _OSF1 || defined _SOLARIS || defined _HPUX)
typedef unsigned long PTR_INT;
#else
typedef unsigned int PTR_INT;
#endif


/*
/////////////////////////////////////////////////////////////////////////////
//
//  Macro:
//
//      DEFINE_CONSTANT_STRING()
//
//  Purpose:
//
//      declare a constant string as "const char *" or "extern const char *" depending on whether or
//	not INITGUID is defined. This allows constant strings to be safely added
//	to a header file without risk of multiply defined symbols if that header is included in a DLL
//	and in a library that the DLL links to. While INITGUID doesn't have anything to do with constant
//	strings, it is a great constant to switch off of because it is already used to denote when a GUID
//	should be defined as external.
//
//  Parameters:
//
//      name
//      The name of the variable to which the constant string will be assigned
//
//      string
//      the constant string (in quotes).
//
//
//
//  NOTES:
//      There is currently no way to use DEFINE_CONSTANT_STRING() to declare an "extern const char*" when
//	INITGUID is defined. This functionality could be added, but until there is a use case, it would just
//	make things more complicated.
//
*/
#if defined (INITGUID)
#define DEFINE_CONSTANT_STRING(name, string) \
    const char *name = string;
#else
#define DEFINE_CONSTANT_STRING(name, string) \
    extern const char* name;
#endif


#endif /* _HXTYPES_H_ */
