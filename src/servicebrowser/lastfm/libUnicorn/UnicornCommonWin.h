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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef UNICORNCOMMONWIN_H
#define UNICORNCOMMONWIN_H

#include "UnicornDllExportMacro.h"

#include <QString>
#include <string>
#include <windows.h>

/** @author <erik@last.fm> */

namespace UnicornUtils
{
    /*********************************************************************/ /**
        Returns the path to the system's Program Files directory inclusive
        of trailing slash.
    **************************************************************************/
    UNICORN_DLLEXPORT std::string
    programFilesPath();
	
    /*********************************************************************/ /**
        Returns true if we're running on a limited user account.
    **************************************************************************/
    UNICORN_DLLEXPORT bool
    isLimitedUser();

    /*********************************************************************/ /**
        Returns the name of the default player. "" if not found.
    **************************************************************************/
    UNICORN_DLLEXPORT QString
    findDefaultPlayer();
    
    /********************************************************************** /**
    * Function......: CreateShortcut
    * Parameters....: lpszFileName - string that specifies a valid file name
    *                 lpszDesc - string that specifies a description for a 
    *                            shortcut
    *                 lpszShortcutPath - string that specifies a path and 
    *                                    file name of a shortcut
    * Returns.......: S_OK on success, error code on failure
    * Description...: Creates a Shell link object (shortcut)
    **************************************************************************/
    UNICORN_DLLEXPORT HRESULT
    createShortcut( /*in*/ LPCTSTR lpszFileName, 
                    /*in*/ LPCTSTR lpszDesc, 
                    /*in*/ LPCTSTR lpszShortcutPath );

}


#endif // UNICORNCOMMONWIN_H
