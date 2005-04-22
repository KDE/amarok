
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

#ifndef HLXSYS_MEMORY_H
#define HLXSYS_MEMORY_H

#if defined(_SYMBIAN)
#include <string.h>
#elif defined(_OPENWAVE)
#include "platform/openwave/hx_op_stdc.h"
#elif !defined(__TCS__) && !defined(_VXWORKS)
#include <memory.h>
#endif

#if defined(_SOLARIS) || defined(_MAC_CFM)
#include <string.h>
#endif

#endif /* HLXSYS_MEMORY_H */
