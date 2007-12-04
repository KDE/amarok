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

#include "tfile_helper.h"

#include <string.h>
#include <strings.h>
#include <wchar.h>


bool
CheckExtensionImpl(const char *fileName, const char *extension)
{
    const char *ext = strrchr(fileName, '.');
    return ext && !strcasecmp(ext, extension);
}

bool
CheckExtensionImpl(const wchar_t *fileName, const wchar_t *extension)
{
    const wchar_t *ext = wcsrchr(fileName, L'.');
    return ext && !wcscasecmp(ext, extension);
}
