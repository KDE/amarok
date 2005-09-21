
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

#ifndef PRINT_H
#define PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

//int print2stdout(const char* pFmt, ...);
//int print2stderr(const char* pFmt, ...);
#define STDOUT print2stdout
#define STDERR print2stderr

#ifdef __cplusplus
}
#endif


#endif /* PRINT_H */
