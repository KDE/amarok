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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "EncodingUtils.h"

#include <string>

using namespace std;

/******************************************************************************
    AnsiToUtf8
******************************************************************************/
int
EncodingUtils::AnsiToUtf8(
    const char* ansi,
    char*       utf8,
    int         nUtf8Size)
{
    WCHAR*  wszValue;          // Unicode value
    size_t  ansi_len;
    size_t  len;

    if (ansi == NULL)
    {
        return 0;
    }

    ansi_len = strlen(ansi);

    // Malloc enough memory for a Unicode string the size of the ansi string
    wszValue = (WCHAR *)malloc((ansi_len + 1) * 2);
    if (wszValue == NULL)
    {
        return 0;
    }
    
    // Convert ANSI string to Unicode
    len = MultiByteToWideChar(CP_ACP,
                              0,
                              ansi,
                              static_cast<int>(ansi_len + 1),
                              wszValue,
                              static_cast<int>((ansi_len + 1) * 2 ));
    if ( len == 0 )
    {
        free(wszValue);
        return 0;
    }

    // Convert Unicode value to UTF-8
    *utf8 = '\0';
    len = UnicodeToUtf8(wszValue, -1, utf8, nUtf8Size);
    if (len == 0 )
    {
        free(wszValue);
        return 0;
    }

    free(wszValue);

    return static_cast<int>(len-1);
}

/******************************************************************************
    UnicodeToUtf8
******************************************************************************/
int
EncodingUtils::UnicodeToUtf8(
    const WCHAR* lpWideCharStr,
    int          cwcChars,
    char*        lpUtf8Str,
    int          nUtf8Size)
{
    const unsigned short*   pwc = (unsigned short *)lpWideCharStr;
    unsigned char*          pmb = (unsigned char  *)lpUtf8Str;
    const unsigned short*   pwce; // ptr to end of wide string
    size_t  cBytes = 0;

    if ( cwcChars >= 0 ) {
        pwce = pwc + cwcChars;
    } else {
        pwce = (unsigned short *)((size_t)-1);
    }

    while ( pwc < pwce ) {
        unsigned short  wc = *pwc++;

        if ( wc < 0x00000080 )
        {
            *pmb++ = (char)wc;
            cBytes++;
        }
        else if ( wc < 0x00000800 )
        {
            *pmb++ = (char)(0xC0 | ((wc >>  6) & 0x1F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        }
        else if ( wc < 0x00010000 )
        {
            *pmb++ = (char)(0xE0 | ((wc >> 12) & 0x0F));
            cBytes++;
            *pmb++ = (char)(0x80 | ((wc >>  6) & 0x3F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        }

        // Reached the end?
        if (wc == L'\0' || cBytes >= (size_t)(nUtf8Size - 3))
        {
            return static_cast<int>(cBytes);
        }
    }

    return static_cast<int>(cBytes);
}


/******************************************************************************
    Utf8ToAnsi
******************************************************************************/
string
EncodingUtils::Utf8ToAnsi(
    const char* pcUTF8Str)
{
    // So, we'll do:
    // UTF8 Multibyte to Unicode
    // Unicode to ANSI Multibyte
    
    if (strlen(pcUTF8Str) == 0)
    {
        return string();
    }

    WCHAR* pcWideStr = NULL;
    char*  pcAnsiStr = NULL;

    // Get number of (wide or ANSI) chars the UTF8 string corresponds to
    int nLength = MultiByteToWideChar(CP_UTF8, 0, pcUTF8Str, -1, pcWideStr, 0);

    if (nLength == 0)
    {
        int nError = GetLastError();
        
        // ERROR_INSUFFICIENT_BUFFER
        // ERROR_INVALID_FLAGS
        // ERROR_INVALID_PARAMETER
        // ERROR_NO_UNICODE_TRANSLATION
        LOG(1, "Couldn't get length of UTF8 string: '" << pcUTF8Str <<
            "'. Returning empty string. Error code: " << nError << "\n");
        return string();
    }

    // Allocate enough space for the wide version
    pcWideStr = new WCHAR[nLength + 1]; // length is in characters, not bytes

    int nStatus = MultiByteToWideChar(CP_UTF8, 0, pcUTF8Str, -1, pcWideStr, nLength);

    if (nStatus == 0)
    {
        int nError = GetLastError();
        LOG(1, "Couldn't convert UTF8 string '" << pcUTF8Str <<
            "' to Unicode. Returning empty string. Error code: " << nError << "\n");
        delete[] pcWideStr;
        return string();
    }

    // Allocate enough space for the ANSI char* version
    pcAnsiStr = new char[nLength + 5]; // 5?

    // Conversion to ANSI (CP_ACP)
    nStatus = WideCharToMultiByte(CP_ACP, 0, pcWideStr, -1, pcAnsiStr, nLength, NULL, NULL);

    if (nStatus == 0)
    {
        int nError = GetLastError();

        // ERROR_INSUFFICIENT_BUFFER
        // ERROR_INVALID_FLAGS
        // ERROR_INVALID_PARAMETER
        LOG(1, "Couldn't convert Unicode string '" << pcWideStr << "' to ANSI." <<
            " Error code: " << nError << "\n");
        delete[] pcWideStr;
        delete[] pcAnsiStr;
        return string();
    }
    
    pcAnsiStr[nLength] = 0;

    string sAnsi(pcAnsiStr);
    
    delete[] pcWideStr;
    delete[] pcAnsiStr;

    return sAnsi;
}
