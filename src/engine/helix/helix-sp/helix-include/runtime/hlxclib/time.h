
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

#ifndef HLXSYS_TIME_H
#define HLXSYS_TIME_H

#if defined(_SYMBIAN)
# include <sys/time.h>
#endif 

#if defined(WIN32_PLATFORM_PSPC)
# include "hxtypes.h"
# include "hlxclib/windows.h"
#elif !defined(WIN32_PLATFORM_PSPC) && !defined(_OPENWAVE)
# include <time.h>
#endif /* !defined(WIN32_PLATFORM_PSPC) && !defined(_OPENWAVE) */

#if defined(_OPENWAVE)
# include "platform/openwave/hx_op_timeutil.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*******************************
 * Types
 */

#if defined(_OPENWAVE)

#define NO_TM_ISDST
typedef U32 time_t;
#define tm op_tm                // XXXSAB any other way for 'struct tm' to
                                // work in a C-includeable file?
struct timeval {
	time_t tv_sec;
	time_t tv_usec;
};


#elif defined(WIN32_PLATFORM_PSPC)

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#define timezone _timezone
extern long _timezone;

#endif /* defined(WIN32_PLATFORM_PSPC) */


/*******************************
 * Helix declarations
 */
long __helix_time(long *t);
struct tm* __helix_localtime(long* timep);
void __helix_tzset();
long __helix_mktime(struct tm* tm);
struct tm *__helix_gmtime(long *timep);
int __helix_gettimeofday(struct timeval *tv, void *tz);
char * __helix_ctime(long *timer);

#if defined(_WINCE)
char * __helix_asctime (struct tm *tm);

/*******************************
 * platform specifics declarations
 */

_inline char * ctime(time_t *timp)
{
    return __helix_ctime((long*)timp);
}

_inline char * asctime (struct tm *tm)
{
    return __helix_asctime(tm);
}

_inline
void _tzset()
{
    __helix_tzset();
}

_inline
struct tm* localtime(time_t* timep)
{
    return __helix_localtime((long *)timep);
}

_inline
long time(time_t *t) 
{
    return __helix_time((long *)t);
}


_inline
long mktime(struct tm* tm)
{
    return __helix_mktime(tm);
}

_inline
struct tm* gmtime(time_t *timep)
{
    return __helix_gmtime((long*)timep);
}

#elif defined(_OPENWAVE)
#define time(t)			__helix_time(t)
#define ctime(t)		__helix_ctime(t)
#define gmtime(t)		__helix_gmtime(t)
#define localtime(t)	__helix_gmtime(t) // XXXSAB is there a _local_ time call?
#define mktime(tm)		__helix_mktime(tm)
#define gettimeofday	__helix_gettimeofday

#define strftime op_strftime

#endif /* defined(WIN32_PLATFORM_PSPC) */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* HLXSYS_TIME_H */
