
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

#ifndef HLXSYS_SYS_STAT_H
#define HLXSYS_SYS_STAT_H

#if !defined(WIN32_PLATFORM_PSPC) && !defined(_OPENWAVE) && !defined(_WINCE)
#if defined(_MACINTOSH) && !defined(_MAC_MACHO)
#include <stat.h>
#else
#include <sys/stat.h>
#endif
#endif /* !defined(WIN32_PLATFORM_PSPC) */

#include "hlxclib/sys/types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Helix implementations */
int __helix_fstat(int filedes, struct stat *buf);
int __helix_stat(const char* pFilename, struct stat *buf);

#if defined(WIN32_PLATFORM_PSPC) || defined(_OPENWAVE)

#ifdef _OPENWAVE
#define S_IFDIR		0x1000	/* specify a directory*/
#endif 

inline
int fstat(int filedes, struct stat *buf)
{
    return __helix_fstat(filedes, buf);
}

inline
int _stat(const char* pFilename, struct _stat *buf)
{
    return __helix_stat(pFilename, (struct stat *)buf);
}

inline
int stat(const char* pFilename, struct stat *buf)
{
    return __helix_stat(pFilename, buf);
}

#endif /* defined(WIN32_PLATFORM_PSPC) */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* HLXSYS_SYS_STAT_H */
