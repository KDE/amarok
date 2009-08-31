/****************************************************************************************
 * Copyright (c) 2004 David Hammerton <crazney@crazney.net>                             *
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

#ifndef _PORTABILITY_H
#define _PORTABILITY_H

#if !defined(WIN32) /* POSIX */

#define SYSTEM_POSIX

#include <sys/types.h>
#include <config-amarok.h>  

#if !defined(HAVE_U_INT64_T) && defined(HAVE_UINT64_T)
 typedef uint64_t u_int64_t;
#endif
#if !defined(HAVE_U_INT32_T) && defined(HAVE_UINT32_T)
 typedef uint32_t u_int32_t;
#endif
#if !defined(HAVE_U_INT16_T) && defined(HAVE_UINT16_T)
 typedef uint16_t u_int16_t;
#endif
#if !defined(HAVE_U_INT8_T) && defined(HAVE_UINT8_T)
 typedef uint8_t u_int8_t;
#endif

#else /* WIN32 */

#define SYSTEM_WIN32

#include <windows.h>
#include <time.h>
typedef INT64 int64_t;
typedef UINT64 u_int64_t;

typedef signed int int32_t;
typedef unsigned int u_int32_t;

typedef signed short int16_t;
typedef unsigned short u_int16_t;

typedef signed char int8_t;
typedef unsigned char u_int8_t;

#endif

#endif /* _PORTABILITY_H */


