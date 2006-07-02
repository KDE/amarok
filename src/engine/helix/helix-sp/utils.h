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
#ifndef _HELIXUTILS_
#define _HELIXUTILS_

int SafeSprintf(char *str, int max, const char *fmt, ...)
#ifdef __GNUC__
__attribute__ ((format (printf, 3, 4)))
#endif
;
char *SafeStrCpy(char *str1, const char *str2, int sz);

#endif
