/***************************************************************************
    copyright            : (C) 2007 by Shane King
    email                : kde@dontletsstart.com
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1, or (at your option) any later version, as published by the Free  *
 *   Software Foundation.                                                  *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef TFILE_HELPER_H
#define TFILE_HELPER_H

#include <config-amarok.h>  

#include <tfile.h>

// need to be able to deal with either the straight forward char version
// or a char/wchar hybrid version of the filename
#ifdef COMPLEX_TAGLIB_FILENAME

#define CheckExtension(filename, extension) \
    (static_cast<const char *>(filename) == 0 \
        ? CheckExtensionImpl(static_cast<const wchar_t *>(filename), L##extension) \
        : CheckExtensionImpl(static_cast<const char *>(filename), extension))

#define TagLibOpenFile(filename, mode) \
    (static_cast<const char *>(filename) == 0 \
        ? _wfopen(filename, L##mode) \
        : fopen(filename, mode))

#include <iostream>
inline std::basic_ostream<char> &operator<<(std::basic_ostream<char> &stream, TagLib::FileName fileName)
{
    if (static_cast<const char *>(fileName) == 0) 
        stream << static_cast<const wchar_t *>(fileName);
    else
        stream << static_cast<const char *>(fileName);
    return stream;
}

#else

#define CheckExtension(filename, extension) CheckExtensionImpl(filename, extension)
#define TagLibOpenFile(filename, mode) fopen(filename, mode)

#endif

bool CheckExtensionImpl(const char *fileName, const char *extension);
bool CheckExtensionImpl(const wchar_t *fileName, const wchar_t *extension);

#endif // TFILE_HELPER_H
