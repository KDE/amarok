/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef LASTFM_WIN_UTILS_H
#define LASTFM_WIN_UTILS_H
#ifdef WIN32

#include <lastfm/DllExportMacro.h>
#include "windows.h"


namespace Utils
{
    /** @returns true if we're running on a limited user account */
    LASTFM_CORE_DLLEXPORT bool isLimitedUser();

    /** Function......: CreateShortcut
      * Parameters....: lpszFileName - string that specifies a valid file name
      *                 lpszDesc - string that specifies a description for a 
      *                            shortcut
      *                 lpszShortcutPath - string that specifies a path and 
      *                                    file name of a shortcut
      * Returns.......: S_OK on success, error code on failure
      * Description...: Creates a Shell link object (shortcut)
      */
    LASTFM_CORE_DLLEXPORT HRESULT createShortcut( /*in*/ LPCTSTR lpszFileName, 
                                           /*in*/ LPCTSTR lpszDesc, 
                                           /*in*/ LPCTSTR lpszShortcutPath );
}

#endif
#endif
