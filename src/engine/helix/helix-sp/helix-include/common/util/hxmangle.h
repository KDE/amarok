
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

#ifndef _HXMANGLE_H_
#define _HXMANGLE_H_

#define CLIENT_GUID_REGNAME "Rotuma"
#define CLIENT_ID_REGNAME   "Futuna"

static const char CLIENT_ZERO_GUID[] = "00000000-0000-0000-0000-000000000000";

// given an input buffer, mangle it and return it back
// The caller has to call delete[] on the returned pointer
char* Cipher(const char* pszBuffer);

// given an input buffer, de-mangle it and return it back
// The caller has to call delete[] on the returned pointer
char* DeCipher(const char* pszBuffer);

#endif // _HXMANGLE_H_
