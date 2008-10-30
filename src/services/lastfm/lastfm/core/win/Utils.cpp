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

#include "Utils.h"
#include "../CoreDir.h"
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QSettings>
#include <QFileInfo>
#include <shlobj.h>
#include <atlbase.h> // For the COM shortcut stuff
#include <string>

using std::string;
using std::vector;


bool
Utils::isLimitedUser()
{
    // If we can write to Program Files we aren't limited
    QFile f( CoreDir::programFiles().filePath( "dummy" ) );
    if (!f.open( QIODevice::WriteOnly ))
    {
        qWarning() << "Couldn't open test file, it's a limited user";
        return true;
    }
    else
    {
        QFile::remove( f.fileName() );
        return false;
    }
}


#if 0
QString
Utils::findDefaultPlayer()
{
    // Get mp3 progID
    QSettings regKey( "HKEY_LOCAL_MACHINE\\Software\\Classes\\.mp3", QSettings::NativeFormat );
    QString progId = regKey.value("Default").toString();

    // Look under progID
    QSettings progIdKey(
        QString( "HKEY_LOCAL_MACHINE\\Software\\Classes\\%1\\shell\\open\\command" ).arg( progId ),
        QSettings::NativeFormat );
    QString exeCmd = progIdKey.value("Default").toString();

    vector<string> separated;
    Unicorn::parseQuotedStrings( exeCmd.toStdString(), separated );

    QString path = separated.size() > 0 ? QString::fromStdString(separated.at(0)) : "";
    QFileInfo file(path);
    return file.fileName();
}
#endif


HRESULT
Utils::createShortcut( LPCTSTR lpszFileName, 
                LPCTSTR lpszDesc, 
                LPCTSTR lpszShortcutPath )
{
    HRESULT hRes = E_FAIL;
    DWORD dwRet = 0;
    CComPtr<IShellLink> ipShellLink;
        // buffer that receives the null-terminated string 
        // for the drive and path
    TCHAR szPath[MAX_PATH];    
        // buffer that receives the address of the final 
        //file name component in the path
    LPTSTR lpszFilePart;    
    WCHAR wszTemp[MAX_PATH];
        
    // Retrieve the full path and file name of a specified file
    dwRet = GetFullPathName(lpszFileName, 
                       sizeof(szPath) / sizeof(TCHAR), 
                       szPath, &lpszFilePart);
    if (!dwRet)                                        
        return hRes;

    // Get a pointer to the IShellLink interface
    hRes = CoCreateInstance(CLSID_ShellLink,
                            NULL, 
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (void**)&ipShellLink);

    if (SUCCEEDED(hRes))
    {
        // Get a pointer to the IPersistFile interface
        CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);

        // Set the path to the shortcut target and add the description
        hRes = ipShellLink->SetPath(szPath);
        if (FAILED(hRes))
            return hRes;

        hRes = ipShellLink->SetDescription(lpszDesc);
        if (FAILED(hRes))
            return hRes;

        // IPersistFile is using LPCOLESTR, so make sure 
        // that the string is Unicode
    #if !defined _UNICODE
        MultiByteToWideChar(CP_ACP, 0, 
                       lpszShortcutPath, -1, wszTemp, MAX_PATH);
    #else
        wcsncpy( wszTemp, lpszShortcutPath, MAX_PATH );
    #endif

        // Write the shortcut to disk
        hRes = ipPersistFile->Save(wszTemp, TRUE);
    }

    return hRes;
}
