
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * Portions (c) Paul Cifarelli 2005
 */

#include <stdarg.h>
#include <stdio.h>
#include "helix-sp.h"
#include "print.h"

int HelixSimplePlayer::print2stdout(const char* pFmt, ...)
{
    va_list args;
    
    va_start(args, pFmt);

    int ret = vfprintf(stdout, pFmt, args);

    va_end(args);

    return ret;
}

int HelixSimplePlayer::print2stderr(const char* pFmt, ...)
{
    va_list args;
    
    va_start(args, pFmt);

    int ret = vfprintf(stderr, pFmt, args);

    va_end(args);

    return ret;
}
