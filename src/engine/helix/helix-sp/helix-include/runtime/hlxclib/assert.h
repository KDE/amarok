
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

#ifndef HLXSYS_ASSERT_H
#define HLXSYS_ASSERT_H

#if defined(_OPENWAVE)
#include "platform/openwave/hx_op_debug.h"
#define assert(x) OpASSERT(x)
#elif !defined(WIN32_PLATFORM_PSPC)
#include <assert.h>
#endif /* !defined(WIN32_PLATFORM_PSPC) */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void __helix_assert(const char* pExpression, 
		    const char* pFilename, int lineNum);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#if defined(WIN32_PLATFORM_PSPC)

#include "hxtypes.h"
#include <winbase.h>
#include <dbgapi.h>

#ifdef _DEBUG
#define assert(x) if(!(x)) __helix_assert(#x, __FILE__, __LINE__);
#else /* _DEBUG */
#define assert(x)
#endif /* _DEBUG */

#endif /* defined(WIN32_PLATFORM_PSPC) */

#endif /* HLXSYS_ASSERT_H */
