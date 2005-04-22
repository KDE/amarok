
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

#ifndef HLXSYS_SYS_TYPES_H
#define HLXSYS_SYS_TYPES_H

#if !defined(WIN32_PLATFORM_PSPC) && !defined(_MACINTOSH) && !defined(_OPENWAVE)
#include <sys/types.h>
#endif /* !defined(WIN32_PLATFORM_PSPC) */

#if defined (_WINDOWS) || (defined (_MACINTOSH) && !defined(_MAC_MACHO))
typedef long off_t;
#endif /* defined (_WINDOWS) || defined (_MACINTOSH) */

#if defined(_OPENWAVE)
#include "hlxclib/time.h"
#include "platform/openwave/hx_op_fs.h"
typedef unsigned short mode_t;
typedef OpFsSize off_t;         // XXXSAB???

#ifndef SEEK_SET
#define SEEK_SET kOpFsSeekSet
#define SEEK_CUR kOpFsSeekCur
#define SEEK_END kOpFsSeekEnd
#endif

struct stat
{
    // XXXSAB fill this in...
    mode_t st_mode;
    off_t st_size;
    time_t st_atime;
    time_t st_ctime;
    time_t st_mtime;
    short st_nlink;
};

#elif defined(WIN32_PLATFORM_PSPC)
#include "hlxclib/windows.h"

typedef unsigned short mode_t;

#define S_IFDIR 0040000

struct stat {
    mode_t st_mode;
    off_t st_size;
    time_t st_atime;
    time_t st_ctime;
    time_t st_mtime;
    short st_nlink;
};

#endif /* defined(WIN32_PLATFORM_PSPC) */

#endif /* HLXSYS_SYS_TYPES_H */
