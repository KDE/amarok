
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

#ifndef HLXSYS_LIMITS_H
#define HLXSYS_LIMITS_H

#ifdef _OPENWAVE_SIMULATOR
#ifndef _WIN32
#define _WIN32
#define LIMITS_UNDEF_WIN32
#endif /* _WIN32 */
#endif /* _OPENWAVE_SIMULATOR */

#include <limits.h>

#ifdef LIMITS_UNDEF_WIN32
#undef _WIN32
#undef LIMITS_UNDEF_WIN32
#endif /* LIMITS_UNDEF_WIN32 */

#endif /* HLXSYS_LIMITS_H */
