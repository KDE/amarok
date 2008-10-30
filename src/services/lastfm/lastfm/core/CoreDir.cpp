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

#include "CoreDir.h"
#include <QDebug>
#include <QDir>
#ifdef WIN32
    #include <shlobj.h>
#endif
#ifdef Q_WS_MAC
    #include <QCoreApplication>
    #include <Carbon/Carbon.h>
#endif


#ifdef Q_WS_MAC
QDir
CoreDir::bundle()
{
    return QDir( qApp->applicationDirPath() ).absoluteFilePath( "../.." );
}
#endif


QDir
CoreDir::dataDotDot()
{
#ifdef WIN32
    if ((QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based) == 0)
    {
        // Use this for non-DOS-based Windowses
        char path[MAX_PATH];
        HRESULT h = SHGetFolderPathA( NULL, 
                                      CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                                      NULL, 
                                      0, 
                                      path );
        if (h == S_OK)
            return QString::fromLocal8Bit( path );
    }
    return QDir::home();

#elif defined(Q_WS_MAC)

    #define EIT( x ) { OSErr err = x; if (err != noErr) throw 1; }
    try
    {
        short vRefNum = 0;
        long dirId;
        EIT( ::FindFolder( kOnAppropriateDisk, 
                           kApplicationSupportFolderType,
                           kDontCreateFolder, 
                           &vRefNum, 
                           &dirId ) );

        // Now we have a vRefNum and a dirID - but *not* an Unix-Path as string.
        // Lets make one based from this:
        FSSpec fsspec;
        EIT( ::FSMakeFSSpec( vRefNum, dirId, NULL, &fsspec ) );

        // ...and build an FSRef based on thes FSSpec.
        FSRef fsref;
        EIT( ::FSpMakeFSRef( &fsspec, &fsref ) );

        // ...then extract the Unix Path as a C-String from the FSRef
        unsigned char path[512];
        EIT( ::FSRefMakePath( &fsref, path, 512 ) );

        return QDir::homePath() + QString::fromUtf8( (char*)path );
    }
    catch (int)
    {
        return QDir::home().filePath( "Library/Application Support" );
    }

#elif defined(Q_WS_X11)
    return QDir::home().filePath( ".local/share" );

#else
    return QDir::home();
#endif
}


#ifdef WIN32
QDir
CoreDir::programFiles()
{
    char path[MAX_PATH];

    // TODO: this call is dependant on a specific version of shell32.dll.
    // Need to degrade gracefully. Need to bundle SHFolder.exe with installer
    // and execute it on install for this to work on Win98.
    HRESULT h = SHGetFolderPathA( NULL,
                                 CSIDL_PROGRAM_FILES, 
                                 NULL,
                                 0, // current path
                                 path );

    if (h != S_OK)
    {
        qCritical() << "Couldn't get Program Files dir. Possibly Win9x?";
        return "";
    }

    return QString::fromLocal8Bit( path );
}
#endif
