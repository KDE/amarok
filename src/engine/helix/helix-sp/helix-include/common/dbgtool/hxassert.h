
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

/////////////////////////////////////////////////////////////////////////////
// HXASSERT.H
//
// Debugging support header.
//
// HX_ASSERT()	- asserts an expression is TRUE. Compiles to no-ops in
//				retail builds. Provides message box or other UI when 
//				expression fails.
//
// HX_ASSERT_VALID_PTR() - asserts that a pointer is valid. Performs more
//				rigid verification specifically appropriate for pointers.
//
// HX_VERIFY()	- verifies an expression is TRUE. Expression or code DOES NOT 
//				compile away in retail builds, but UI of failure is removed.
//				In debug builds provides message box or other UI when 
//				expression fails.
//
// HX_TRACE()	- Similar to DEBUGPRINTF() but no buffer is required. 
//				Compiles to no-ops in retail builds.
//

#ifndef _HXASSERT_H_
#define _HXASSERT_H_

#include "hlxclib/assert.h"

#ifndef ASSERT
#if defined (DEBUG) || defined (_DEBUG)
#if defined (_OSF1) && defined (_NATIVE_COMPILER)
#	define ASSERT(x)	assert(((long)(x)) != 0L)
#else
#       define ASSERT(x)        assert(x)
#endif
#else
#	define ASSERT(x)	/* x */
#endif /* DEBUG */
#endif /* ndef ASSERT */

#include "hlxclib/limits.h"
#include "hxtypes.h"
#include "hxresult.h"           // for HX_RESULT
#include "hlxclib/stdio.h"      // for sprintf

#ifdef _SUN
#include <stddef.h>     // for size_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (_SOLARIS) || defined (_IRIX) || defined (_DECALPHA)
#ifndef __inline
#define __inline inline
#endif
#endif

#if defined (_OSF1) && defined (_NATIVE_COMPILER)
#if defined __cplusplus
#define __inline inline
#else
#define __inline static
#endif /* __cplusplus */
#endif

#ifdef _HPUX 
#if defined __cplusplus
#define __inline inline
#else
#define __inline 
#endif /* __cplusplus */
#endif

#if defined _AIX
#if defined __cplusplus
#define __inline inline
#else
#define __inline 
#endif /* __cplusplus */
#endif /* _AIX */

#ifdef _UNIX
#include <signal.h> /* For kill() */
#include <unistd.h> /* For getpid() */
#include <stdio.h>
void HXUnixDebugBreak();
#endif


/////////////////////////////////////////////////////////////////////////////
// Diagnostic support

// For _MAX_PATH
#if defined ( _MACINTOSH )
#include "platform/mac/maclibrary.h"
#elif defined (_UNIX)
#include <stdlib.h>
#if !defined(_VXWORKS)
#include <sys/param.h>
#endif
#define _MAX_PATH       MAXPATHLEN
#elif defined (_WINDOWS)
#include <stdlib.h>
#endif

#ifdef _SYMBIAN
# include <unistd.h>
# define _MAX_PATH MAXPATHLEN
#endif

#ifdef _OPENWAVE
#ifndef _MAX_PATH
#define _MAX_PATH 256
#endif
#endif

#if defined (DEBUG) || defined (_DEBUG)

#ifdef _MACINTOSH
#  define MAX_TRACE_OUTPUT  255
#else
# ifdef _UNIX
#  define MAX_TRACE_OUTPUT  255
# elif _WIN16
#  define MAX_TRACE_OUTPUT  255
# elif _SYMBIAN
#  define MAX_TRACE_OUTPUT  255
# else
#  define MAX_TRACE_OUTPUT  (_MAX_PATH*2 + 20)
# endif
#endif


/////////////////////////////////////////////////////////////////////////////
//
//	HXDebugOptionEnabled: 
//		Determine if the given debug option is enabled.
//		A lookup is done to the registry key, and if it's present
//		and set to '1', TRUE is returned otherwise FALSE
//		Similar to HXWantTraceMessages, except not specific to one option.
#ifdef _WIN16
// The STDMETHODCALLTYPE includes the _export keyword, even though this is
// a static, not dll, library.  Seems to work on win32 ok, but not a win16
// .exe.  rpapp would see this as duplicate symbol, until the _export was
// removed, and all the libraries that rpapp linked with rebuilt.  There
// may be a define for STDMETHODCALLTYPE without the _export that should be
// used here. XXXTW.  april 98.
HXBOOL far _cdecl HXDebugOptionEnabled(const char* szOption);
#else
HXBOOL STDMETHODCALLTYPE HXDebugOptionEnabled(const char* szOption);
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	HXWantTraceMessages: 
//		Helper function used to determine if the system has asked for trace 
//		messages.
//
HXBOOL STDMETHODCALLTYPE HXWantTraceMessages();

/////////////////////////////////////////////////////////////////////////////
//
//	HXOutputDebugString: 
//		Helper function used by DEBUGOUTSTR(). This is better than 
//		OutputDebugString, because it will check to see if output
//		tracing is turned off in the registry. This prevents the massive
//		slew of output messages.
//
void STDMETHODCALLTYPE HXOutputDebugString(const char* pString);

/////////////////////////////////////////////////////////////////////////////
//
// HXTrace: Helper function used by HX_TRACE()
//
void STDMETHODVCALLTYPE HXTrace(const char* pszFormat, ...);


/////////////////////////////////////////////////////////////////////////////
//
// HXAssertFailedLine: Helper function used by HX_ASSERT()
//
#ifdef _WIN16
// The STDMETHODCALLTYPE includes the _export keyword, even though this is
// a static, not dll, library.  Seems to work on win32 ok, but not a win16
// .exe.  rpapp would see this as duplicate symbol, until the _export was
// removed, and all the libraries that rpapp linked with rebuilt.  There
// may be a define for STDMETHODCALLTYPE without the _export that should be
// used here. XXXTW.  april 98.
HXBOOL far _cdecl HXAssertFailedLine(const char* pszExpression, const char* pszFileName, int nLine);
#else
HXBOOL STDMETHODCALLTYPE HXAssertFailedLine(const char* pszExpression, const char* pszFileName, int nLine);
#endif

/////////////////////////////////////////////////////////////////////////////
//
// HXAssertValidPointer, HXIsValidAddress: Helper functions used by
// HX_ASSERT_VALID_PTR()
//
void STDMETHODCALLTYPE HXAssertValidPointer(const void* pVoid, const char* pszFileName, int nLine);
#ifdef __cplusplus
#ifdef _WIN16
    // see note above on the problem with STDMETHODCALLTYPE on win16
    HXBOOL far _cdecl        HXIsValidAddress(const void* lp, ULONG32 nBytes = 1, HXBOOL bReadWrite = TRUE);
#else
    HXBOOL STDMETHODCALLTYPE HXIsValidAddress(const void* lp, ULONG32 nBytes = 1, HXBOOL bReadWrite = TRUE);
#endif
#else
#ifdef _WIN16
    // see note above on the problem with STDMETHODCALLTYPE on win16
    HXBOOL far _cdecl        HXIsValidAddress(const void* lp, ULONG32 nBytes, HXBOOL bReadWrite);
#else
    HXBOOL STDMETHODCALLTYPE HXIsValidAddress(const void* lp, ULONG32 nBytes, HXBOOL bReadWrite);
#endif
#endif

#ifdef _WIN16
HXBOOL far _cdecl        HXIsValidString(const char* lpsz, int nLength);
#else
HXBOOL STDMETHODCALLTYPE HXIsValidString(const char* lpsz, int nLength);
#endif

/////////////////////////////////////////////////////////////////////////////
//
// HXDebugBreak: used to break into debugger at critical times
//
#ifndef HXDebugBreak
// by default, debug break is asm int 3, or a call to DebugBreak, or nothing

#if defined(_WINDOWS)
#if !defined(_M_IX86)
#include "windows.h"
#define HXDebugBreak() DebugBreak()
#else
#define HXDebugBreak() _asm { int 3 }
#endif // _M_ALPHA
#elif defined( _MACINTOSH )
#define HXDebugBreak() Debugger()
#elif defined(_OPENWAVE)
#if defined(_OPENWAVE_SIMULATOR) // windows app...
#define HXDebugBreak() _asm { int 3 }
#else
void HXDebugBreak();
#endif
#elif defined(_SYMBIAN)
#if defined(__WINS__)
#define HXDebugBreak() _asm { int 3 }
#else
void HXDebugBreak();
#endif //_SYMBIAN
#elif  defined(_UNIX)
void HXDebugBreak();
#elif defined(_VXWORKS)
#include <taskLib.h>
#define HXDebugBreak() (taskSuspend(taskIdSelf()))

#endif // end of#if defined(_WIN32) || defined(_WINDOWS)

#endif // end of#ifndef HXDebugBreak

#ifdef _UNIX
#include "signal.h"
#include "stdlib.h"

#define HX_ENABLE_JIT_DEBUGGING()               \
do {						\
    signal (SIGSEGV, (void (*)(int))HXDebugBreak);		\
    signal (SIGBUS,  (void (*)(int))HXDebugBreak);		\
    signal (SIGFPE,  (void (*)(int))HXDebugBreak);		\
    if (!getenv("PROCESS_NAME"))                \
    {                                           \
    char *progname = new char[strlen(argv[0]) + 30]; \
    sprintf(progname, "PROCESS_NAME=%s", argv[0]); /* Flawfinder: ignore */ \
    putenv(progname);                           \
    } \
} while (0);
#else
#define HX_ENABLE_JIT_DEBUGGING()
#endif

/////////////////////////////////////////////////////////////////////////////
//
// HXAbort: used to shut down the application at critical times
//
#ifndef HXAbort

#if (defined(_WIN32) || defined(_WINDOWS)) && !defined(WIN32_PLATFORM_PSPC)
# define HXAbort() 	abort()
#elif defined(WIN32_PLATFORM_PSPC)
# define HXAbort()   exit(1)
#elif defined(_SYMBIAN)
# define HXAbort() exit(1)
#elif defined(_OPENWAVE)
// XXXSAB is this right??
# define HXAbort() exit(1)
#elif defined ( _MACINTOSH )
# define HXAbort() DebugStr("\pHXAbort: Please exit this program.")
#elif defined ( _UNIX )
# define HXAbort() printf("\npnabort: Please exit this program.\n")
#endif // end of#if defined(_WIN32) || defined(_WINDOWS)

#endif // end of#ifndef HXAbort


/////////////////////////////////////////////////////////////////////////////
//
// HX_TRACE: see above.
//
#define HX_TRACE			::HXTrace

/////////////////////////////////////////////////////////////////////////////
//
// HX_ASSERT: see above.
//
#define HX_ASSERT(f) \
	do \
	{ \
	if (!(f) && HXAssertFailedLine(#f, __FILE__, __LINE__)) \
		HXDebugBreak(); \
	} while (0) \


#define REQUIRE_REPORT(targ,file,line)	HXAssertFailedLine(targ,file,line)




/////////////////////////////////////////////////////////////////////////////
//
//	Macros Defined for logging messges in the player. 
//  
//  STARTLOGSINK: Query for an IHXErrorSink
//
//  LOGINFO: Report the error to the error sink
//
//  STOPLOGSINK: Shutdown and Release the error sink.	       
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _GOLD

#ifndef _INTERFACE
#define _INTERFACE struct
#endif

typedef _INTERFACE IHXErrorMessages IHXErrorMessages;

    const char*	    FileAndLine(const char* pFile, UINT32 ulLine);
    HX_RESULT	    HXLogf(IHXErrorMessages *pErr, UINT8 sev, HX_RESULT err, UINT32 usr, const char* pURL, const char* pUsrStr, ...);
    HX_RESULT	    HXLog(IHXErrorMessages *pErr, UINT8 sev, HX_RESULT err, UINT32 usr, const char* pStr, const char* pURL);

#define THIS_LINE() FileAndLine(__FILE__, __LINE__)

#define HX_STARTLOG(__IUnknown, __ErrorMessages) \
	__IUnknown->QueryInterface(IID_IHXErrorMessages, (void **)&__ErrorMessages); \
	HX_ASSERT(__ErrorMessages != NULL && "Must have IHXErrorMessages interface to start logging")

#define HX_LOG HXLog
#define HX_LOGF HXLogf

#define HX_STOPLOG(__ErrorMessages)	HX_RELEASE(__ErrorMessages)

#else // _GOLD
#define HX_STARTLOG(x,y)    (void)0
#define HX_LOG		1 ? (void)0 : HXLog
#define HX_LOGF		1 ? (void)0 : HXLogf
#define HX_STOPLOG(x)	(void)0
#endif

/////////////////////////////////////////////////////////////////////////////
//
// HX_SAFESIZE_T
//
#define HX_SAFESIZE_T(f) \
	( ((ULONG32)(f) <= (size_t)-1)?((size_t)(f)): \
		(HXAssertFailedLine("size_t overflow",__FILE__, __LINE__), (size_t)(f)) ) \

/////////////////////////////////////////////////////////////////////////////
//
// HX_SAFEINT
//
#ifndef _VXWORKS
#define HX_SAFEINT(f) \
	( ((LONG32)(f) <= LONG_MAX && ((LONG32)(f)) >= ((LONG32)(LONG_MIN)))?((int)(f)): \
		(HXAssertFailedLine("Integer Size Overflow",__FILE__, __LINE__), (int)(f)) ) \

#else

#ifdef __cplusplus
extern "C"
#endif
int safe_int_func_call(LONG32 f);
#define HX_SAFEINT(f) safe_int_func_call((LONG32)(f))
#endif

#define HX_SAFEUINT(f) \
	( ((ULONG32)(f) <= ULONG_MAX && (LONG32)(f) >= 0)?((unsigned int)(f)): \
		(HXAssertFailedLine("Unsigned Integer Size Overflow",__FILE__, __LINE__), (unsigned int)(f)) ) \

#define HX_SAFEINT16(f) \
	( ((LONG32)(f) <= SHRT_MAX && (LONG32)(f) >= SHRT_MIN)?((short)(f)): \
		(HXAssertFailedLine("Short Integer Size Overflow",__FILE__, __LINE__), (short)(f)) ) \

#define HX_SAFEUINT16(f) \
	( ((ULONG32)(f) <= USHRT_MAX && (LONG32)(f) >= 0)?((unsigned short)(f)): \
		(HXAssertFailedLine("Unsigned Short Integer Size Overflow",__FILE__, __LINE__), (unsigned short)(f)) ) \

#define HX_SAFEINT8(f) \
	( ((LONG32)(f) <= SCHAR_MAX && (LONG32)(f) >= SCHAR_MIN)?((char)(f)): \
		(HXAssertFailedLine("Signed Char Size Overflow",__FILE__, __LINE__), (char)(f)) ) \

#define HX_SAFEUINT8(f) \
	( ((ULONG32)(f) <= UCHAR_MAX && (LONG32)(f) >= 0)?((unsigned char)(f)): \
		(HXAssertFailedLine("Unsigned Char Size Overflow",__FILE__, __LINE__), (unsigned char)(f)) ) \



	
/////////////////////////////////////////////////////////////////////////////
//
// HX_SAFE_VOID2HANDLE
//
#if defined(_WINDOWS)

#ifdef _WIN32
#define HX_SAFE_VOID2HANDLE(f) ((HANDLE)(f))
#else // !_WIN23
#define HX_SAFE_VOID2HANDLE(f) ((HANDLE)LOWORD(f))
// this doesn't work most of the time since the assignment of a handle (near *) to a
// void * sets the high word to the SS.
//	( ( (0xFFFF0000 & ((ULONG32)(f))) == 0)?((HANDLE)LOWORD(f)): 
//		(HXAssertFailedLine("HANDLE HIWORD NOT NULL",__FILE__, __LINE__), (HANDLE)LOWORD(f)) ) 

#endif

#elif defined ( __MWERKS__ )
#define HX_SAFE_VOID2HANDLE(f)	(f)
#elif defined ( _UNIX )
#define HX_SAFE_VOID2HANDLE(f)	(f)
#endif // end of#if defined(_WIN32) || defined(_WINDOWS)

/////////////////////////////////////////////////////////////////////////////
//
// HX_VERIFY: see above.
//
#define HX_VERIFY(f)          HX_ASSERT(f)

/////////////////////////////////////////////////////////////////////////////
//
// HX_ASSERT_VALID_PTR: see above.
//
#define HX_ASSERT_VALID_PTR(pOb)  (::HXAssertValidPointer((const void*)pOb, __FILE__, __LINE__))
#define HX_ASSERT_VALID_READ_PTR(pOb)  HX_ASSERT(::HXIsValidAddress(pOb, 1, FALSE))

/////////////////////////////////////////////////////////////////////////////
//
// _SAFESTRING: Similar to HX_TRACE() except that it assume you are
// using a single format string, with a single parameter string, this will
// ensure that the length of the resulting format is less than the maximum
// trace string, and gracefully handle the situation...
//
#define HX_TRACE_SAFESTRING(f,s)\
	if ((strlen(f) + strlen(s)) < (size_t)MAX_TRACE_OUTPUT)\
	{\
		HX_TRACE(f,s);\
	}\
	else\
	{\
		HX_TRACE(f,"Some really big URL");\
	}\

#else   // _DEBUG

#ifndef _DEBUG

#ifndef HX_ENABLE_JIT_DEBUGGING
#define HX_ENABLE_JIT_DEBUGGING()
#endif

#ifdef HXAbort
#undef HXAbort
#endif // end of#ifdef HXAbort

#define HXAbort()

#endif  // _DEBUG

#ifndef _DEBUG

#ifdef HXDebugBreak
#undef HXDebugBreak
#endif // end of#ifdef HXDebugBreak

#define HXDebugBreak()

#endif  // _DEBUG || DEBUG

/////////////////////////////////////////////////////////////////////////////
//
// HX_SAFEINT
//
#define HX_SAFEINT(f) ((int)(f)) 
#define HX_SAFEUINT(f) ((unsigned int)(f)) 
#define HX_SAFEINT16(f) ((short)(f)) 
#define HX_SAFEUINT16(f) ((unsigned short)(f)) 
#define HX_SAFEINT8(f) ((char)(f)) 
#define HX_SAFEUINT8(f) ((unsigned char)(f)) 

#define HX_ASSERT(f)			        ((void)0)
#define HX_SAFESIZE_T(f)			((size_t)(f))
#define HX_SAFEINT(f)				((int)(f))
#define HX_SAFE_VOID2HANDLE(f)		((HANDLE)(ULONG32)(f))
#define HX_ASSERT_VALID_PTR(pOb)	((void)0)
#define HX_ASSERT_VALID_READ_PTR(pOb)   ((void)0)
#define HXOutputDebugString(f)			((void)0)

#define REQUIRE_REPORT(targ,file,line)	((int)0)

#if defined (_MACINTOSH)
// this is the proper release version of HX_VERIFY that preserves the syntactic
// role of the debug version; it's necessary to quiet warnings on the Mac
#define HX_VERIFY(f)				do { if (!(f)) {} } while (0)
#else
#define HX_VERIFY(f)				((void)(f))
#endif

#if defined (_WINDOWS)
__inline void __cdecl HXTrace(const char* x, ...) { }
#else
static __inline void HXTrace(const char* /* x */, ...) {}
#endif

#define HX_TRACE              		1 ? (void)0 : ::HXTrace
#define HX_TRACE_SAFESTRING			HX_TRACE

#endif // !_DEBUG

#ifdef __cplusplus
}	// end extern "C"
#endif

#ifdef __cplusplus
/////////////////////////////////////////////////////////////////////////////
//
// Helper for loginfo for LOGINFO()
//
#ifndef _INTERFACE
#define _INTERFACE struct
#endif

typedef _INTERFACE IHXErrorSink IHXErrorSink;

class LogInfo
{
    public:
	static void STDMETHODCALLTYPE FileandLine(const char* file, int nLine);
	static void STDMETHODCALLTYPE Report(IHXErrorSink* mySink, const char* pUserInfo, ...);
    private:
	static char m_pFile[_MAX_PATH]; /* Flawfinder: ignore */
};

#endif


/////////////////////////////////////////////////////////////////////////////
// CHECK/REQUIRE MACROS
//
// These macros are always valid (debug and release builds)
//
//

//
// REQUIRE and REQUIRE_ACTION _always_ emit code for the goto if the statement is false
//
// REQUIRE_REPORT only generates code in Debug builds
//
// The do {} while (0) construct ensures this is legal wherever a statement is legal
//

#define CHECK(stmt)  HX_ASSERT(stmt)

#define REQUIRE_VOID_RETURN(stmt) \
	do { if ((stmt) == 0) { if (REQUIRE_REPORT(#stmt,__FILE__,__LINE__)) HXDebugBreak(); return; } } while (0)
#define REQUIRE_RETURN(stmt,returned) \
	do { if ((stmt) == 0) { if (REQUIRE_REPORT(#stmt,__FILE__,__LINE__)) HXDebugBreak(); return (returned); } } while (0)
#define REQUIRE_VOID_RETURN_QUIET(stmt) \
	do { if ((stmt) == 0) { return; } } while (0)
#define REQUIRE_RETURN_QUIET(stmt,returned) \
	do { if ((stmt) == 0) { return (returned); } } while (0)
#define REQUIRE(stmt,target) \
	do { if ((stmt) == 0) { REQUIRE_REPORT(#target,__FILE__,__LINE__); goto target; } } while (0)
#define REQUIRE_ACTION(stmt,target,action) \
	do { if ((stmt) == 0) { REQUIRE_REPORT(#target,__FILE__,__LINE__); {{action;} goto target;} } } while (0)
#define REQUIRE_QUIET(stmt,target) \
	do { if ((stmt) == 0) goto target; } while (0)
#define REQUIRE_ACTION_QUIET(stmt,target,action) \
	do { if ((stmt) == 0) {{action;} goto target;} } while (0)
#define PRE_REQUIRE_RETURN(stmt,returned) \
	REQUIRE_RETURN(stmt,returned)
#define PRE_REQUIRE_VOID_RETURN(stmt) \
	REQUIRE_VOID_RETURN(stmt)
#define POST_REQUIRE_RETURN(stmt,returned) \
	REQUIRE_RETURN(stmt,returned)
#define POST_REQUIRE_VOID_RETURN(stmt) \
	REQUIRE_VOID_RETURN(stmt)

#define REQUIRE_SUCCESS_RETURN_QUIET(expr) \
	do { register HX_RESULT const res = expr; if (FAILED (res)) return res; } while (0)
#define REQUIRE_SUCCESS_RETURN(expr) \
	do { register HX_RESULT const res = expr; if (FAILED (res)) {  REQUIRE_REPORT("False condition, Aborting...",__FILE__,__LINE__); return res; } } while (0)

//
// REQUIRE_SUCCESS reports the error if an expected result failed
// Ideally, this should report the status value as well
//

#define CHECK_SUCCESS(stat) HX_ASSERT(((unsigned long)(stat)>>31) == 0)

#define REQUIRE_SUCCESS(stat,target) \
	do { if (((unsigned long)(stat)>>31) != 0) { REQUIRE_REPORT(#target,__FILE__,__LINE__); goto target; } } while (0)
#define REQUIRE_SUCCESS_ACTION(stat,target,action) \
	do { if (((unsigned long)(stat)>>31) != 0) { REQUIRE_REPORT(#target,__FILE__,__LINE__); {{action;} goto target;} } } while (0)
#define REQUIRE_SUCCESS_QUIET(stat,target) \
	do { if (((unsigned long)(stat)>>31) != 0) goto target; } while (0)
#define REQUIRE_SUCCESS_ACTION_QUIET(stat,target,action) \
	do { if (((unsigned long)(stat)>>31) != 0) {{action;} goto target;} } while (0)

//
// REQUIRE_NOERR reports the error if the error value is non-zero
// Ideally, this should report the error value as well
//

#define CHECK_NOERR(err) HX_ASSERT((err) == 0)

#define REQUIRE_NOERR_RETURN(err,returned) \
	do { if ((err) != 0) { REQUIRE_REPORT("Toolbox error, Aborting...",__FILE__,__LINE__); return (returned); } } while (0)
#define REQUIRE_NOERR(err,target) \
	do { if ((err) != 0) { REQUIRE_REPORT(#target,__FILE__,__LINE__); goto target; } } while (0)
#define REQUIRE_NOERR_ACTION(err,target,action) \
	do { if ((err) != 0) { REQUIRE_REPORT(#target,__FILE__,__LINE__); {{action;} goto target;} } } while (0)
#define REQUIRE_NOERR_QUIET(err,target) \
	do { if ((err) != 0) goto target; } while (0)
#define REQUIRE_NOERR_ACTION_QUIET(err,target,action) \
	do { if ((err) != 0) {{action;} goto target;} } while (0)

//
// REQUIRE_NONNULL reports the error if the ptr value is null
// Ideally, this should report the error value as well
//
#define CHECK_NONNULL(ptr)	HX_ASSERT((ptr) != 0L)
#define CHECK_NULL(ptr)		HX_ASSERT((ptr) == 0L)

#define REQUIRE_NONNULL_VOID_RETURN(ptr) \
	do { if ((ptr) == 0L) { REQUIRE_REPORT(#ptr" is nil, Aborting...",__FILE__,__LINE__); return; } } while (0)
#define REQUIRE_NONNULL_RETURN(ptr,returned) \
	do { if ((ptr) == 0L) { REQUIRE_REPORT(#ptr" is nil, Aborting...",__FILE__,__LINE__); return (returned); } } while (0)
#define REQUIRE_NONNULL(ptr,target) \
	do { if ((ptr) == 0L) { REQUIRE_REPORT(#target,__FILE__,__LINE__); goto target; } } while (0)
#define REQUIRE_NONNULL_ACTION(ptr,target,action) \
	do { if ((ptr) == 0L) { REQUIRE_REPORT(#target,__FILE__,__LINE__); {{action;} goto target;} } } while (0)
#define REQUIRE_NONNULL_QUIET(ptr,target) \
	do { if ((ptr) == 0L) goto target; } while (0)
#define REQUIRE_NONNULL_ACTION_QUIET(ptr,target,action) \
	do { if ((ptr) == 0L) {{action;} goto target;} } while (0)
// lower case versions make source code more readable

#if defined(_CARBON) || defined(_MAC_MACHO)
#undef check
#undef require
#undef require_action
#undef require_quiet
#undef check_noerr
#undef require_action_quiet
#undef require_noerr
#undef require_noerr_action
#undef require_noerr_quiet
#undef require_noerr_action_quiet
#endif

#define check(stmt) 										CHECK(stmt)

#define require_void_return(stmt)							REQUIRE_VOID_RETURN(stmt)
#define require_return_void(stmt)							REQUIRE_VOID_RETURN(stmt)
#define require_return(stmt,returned)						REQUIRE_RETURN(stmt,returned)
#define require(stmt,target) 								REQUIRE(stmt,target)
#define require_action(stmt,target,action) 					REQUIRE_ACTION(stmt,target,action)
#define require_quiet(stmt,target) 							REQUIRE_QUIET(stmt,target)
#define require_action_quiet(stmt,target,action)			REQUIRE_ACTION_QUIET(stmt,target,action)

#define check_success(stat)									CHECK_SUCCESS(stat)

#define require_success(stat,target) 						REQUIRE_SUCCESS(stat,target)
#define require_success_action(stat,target,action) 			REQUIRE_SUCCESS_ACTION(stat,target,action)
#define require_success_quiet(stat,target) 					REQUIRE_SUCCESS_QUIET(stat,target)
#define require_success_action_quiet(stat,target,action)	REQUIRE_SUCCESS_ACTION_QUIET(stat,target,action)

#define check_noerr(err)									CHECK_NOERR(err)

#define require_noerr_return(err,returned)					REQUIRE_NOERR_RETURN(err,returned)
#define require_noerr(err,target) 							REQUIRE_NOERR(err,target)
#define require_noerr_action(err,target,action) 			REQUIRE_NOERR_ACTION(err,target,action)
#define require_noerr_quiet(err,target) 					REQUIRE_NOERR_QUIET(err,target)
#define require_noerr_action_quiet(err,target,action)		REQUIRE_NOERR_ACTION_QUIET(err,target,action)

#define check_nonnull(ptr)									CHECK_NONNULL(ptr)
#define check_null(ptr)										CHECK_NULL(ptr)

#define require_nonnull_void_return(ptr)					REQUIRE_NONNULL_VOID_RETURN(ptr)
#define require_nonnull_return(ptr,returned)				REQUIRE_NONNULL_RETURN(ptr,returned)
#define require_nonnull(ptr,target) 						REQUIRE_NONNULL(ptr,target)
#define require_nonnull_action(ptr,target,action) 			REQUIRE_NONNULL_ACTION(ptr,target,action)
#define require_nonnull_quiet(ptr,target) 					REQUIRE_NONNULL_QUIET(ptr,target)
#define require_nonnull_action_quiet(ptr,target,action)		REQUIRE_NONNULL_ACTION_QUIET(ptr,target,action)


#endif // !_HXASSERT_H_
