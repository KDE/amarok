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

#ifndef COMMON_FILE_LOCATIONS_H
#define COMMON_FILE_LOCATIONS_H

// relative to UnicornDir::log()
#define MOOSE_LOG_NAME "Last.fm.log"
#define TWIDDLY_LOG_NAME "Twiddly.log"


// relative to qApp->applicationFilePath()
#ifdef WIN32
    #define RELATIVE_PATH_TO_INSTALLED_TWIDDLY_EXE "../Resources/iPodScrobbler"
#endif
#ifdef Q_WS_MAC
    #define RELATIVE_PATH_TO_INSTALLED_TWIDDLY_EXE "iPodScrobbler.exe"
#endif

#endif
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

#ifndef COMMON_FILE_LOCATIONS_H
#define COMMON_FILE_LOCATIONS_H

// relative to UnicornDir::log()
#define MOOSE_LOG_NAME "Last.fm.log"
#define TWIDDLY_LOG_NAME "Twiddly.log"


// relative to qApp->applicationFilePath()
#ifdef WIN32
    #define RELATIVE_PATH_TO_INSTALLED_TWIDDLY_EXE "../Resources/iPodScrobbler"
#endif
#ifdef Q_WS_MAC
    #define RELATIVE_PATH_TO_INSTALLED_TWIDDLY_EXE "iPodScrobbler.exe"
#endif

#endif
