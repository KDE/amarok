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

#if defined(__APPLE__) || defined(__FreeBSD__)
int wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
     int i;

     for (i = 0;
	  s1[i] != L'\0' && s2[i] != L'\0';
	  i++) {
	  wint_t x = towlower(s1[i]);
	  wint_t y = towlower(s2[i]);
	  if (x != y)
	       return x - y;
     }
     return towlower(s1[i]) - towlower(s2[i]);
}
#endif


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
