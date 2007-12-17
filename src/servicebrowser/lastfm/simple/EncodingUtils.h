/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#ifndef ENCODINGUTILS_H
#define ENCODINGUTILS_H

#define USE_LOGGER (0)

#if USE_LOGGER
    #include "Logger.h"
#else
    #include <windows.h>
    #define LOG(x, y)
#endif

#include <string>

/*************************************************************************/ /**
    A bunch of static functions for converting between different character
    sets.

    @author <erik@last.fm>
******************************************************************************/
class EncodingUtils
{
public:

    /*********************************************************************/ /**
        Converts an ANSI string to a UTF-8 string.
        
        @param[in] ansi
    **************************************************************************/
    static int
    AnsiToUtf8(
        const char* ansi,
        char*       utf8,
        int         nUtf8Size);
        
    static int
    UnicodeToUtf8(
        const WCHAR* lpWideCharStr,
        int          cwcChars,
        char*        lpUtf8Str,
        int          nUtf8Size);

    static std::string
    Utf8ToAnsi(
        const char* pcUTF8Str);

	
private:


};

#endif // ENCODINGUTILS_H