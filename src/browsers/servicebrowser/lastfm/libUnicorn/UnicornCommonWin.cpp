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

#include "UnicornCommonWin.h"
#include "UnicornCommon.h"

// This file contains legacy code that doesn't work with Unicode.
//#undef UNICODE
//#undef _UNICODE

#undef LOG
#undef LOGL
#define LOG(x, y)
#define LOGL(x, y)

#include <QFile>
#include <QIODevice>
#include <QSettings>
#include <QFileInfo>

#include <shlobj.h>
//#include <atlbase.h> // For the COM shortcut stuff

#include <string>

using namespace std;

namespace UnicornUtils
{


string
programFilesPath()
{
    char acPath[MAX_PATH];

    // TODO: this call is dependant on a specific version of shell32.dll.
    // Need to degrade gracefully. Need to bundle SHFolder.exe with installer
    // and execute it on install for this to work on Win98.
    HRESULT h = SHGetFolderPathA(NULL,
                                 CSIDL_PROGRAM_FILES,
                                 NULL,
                                 0, // current path
                                 acPath);

    if (h != S_OK)
    {
        LOG(1, "Couldn't get Program Files dir, is this possibly Win 9x?\n");

        //throw logic_error("Couldn't get Program Files dir.");

        acPath[0] = '\0';
        return acPath;
    }

    string sPath(acPath);

    if (sPath[sPath.size() - 1] != '\\')
    {
        sPath.append( "/" );
    }

    return sPath;
}

/******************************************************************************
    GetTempPath
******************************************************************************/
/* use QDir::tempPath instead
string
CWinUtils::GetTempDir()
{
    TCHAR acTempPath[MAX_PATH];
    ::GetTempPath(MAX_PATH, acTempPath);

    string sTempPath(acTempPath);

    if (sTempPath[sTempPath.size() - 1] != '\\')
    {
        sTempPath.append("\\");
    }

    return sTempPath;
}
*/

bool
isLimitedUser()
{
    // Try and write to Program Files, if so we should be fine.
    string pf = programFilesPath();
    QString file = QString(pf.c_str()) + "dummy";
    QFile f( file );
    if ( !f.open( QIODevice::WriteOnly ) )
    {
        LOG(3, "Couldn't open test file, it's a limited user.\n");
        return true;
    }
    else
    {
        f.close();
        QFile::remove(file);
        return false;
    }
}

QString
findDefaultPlayer()
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
    UnicornUtils::parseQuotedStrings( std::string(exeCmd.toAscii().constData()), separated );

    QString path = separated.size() > 0 ? QString(separated.at(0).c_str()) : "";
    QFileInfo file(path);
    return file.fileName();
}


HRESULT
createShortcut( LPCTSTR lpszFileName,
                LPCTSTR lpszDesc,
                LPCTSTR lpszShortcutPath )
{
    HRESULT hRes = E_FAIL;
    //DWORD dwRet = 0;
    //CComPtr<IShellLink> ipShellLink;
    //    // buffer that receives the null-terminated string
    //    // for the drive and path
    //TCHAR szPath[MAX_PATH];
    //    // buffer that receives the address of the final
    //    //file name component in the path
    //LPTSTR lpszFilePart;
    //WCHAR wszTemp[MAX_PATH];
    //
    //// Retrieve the full path and file name of a specified file
    //dwRet = GetFullPathName(lpszFileName,
    //                   sizeof(szPath) / sizeof(TCHAR),
    //                   szPath, &lpszFilePart);
    //if (!dwRet)
    //    return hRes;

    //// Get a pointer to the IShellLink interface
    //hRes = CoCreateInstance(CLSID_ShellLink,
    //                        NULL,
    //                        CLSCTX_INPROC_SERVER,
    //                        IID_IShellLink,
    //                        (void**)&ipShellLink);

    //if (SUCCEEDED(hRes))
    //{
    //    // Get a pointer to the IPersistFile interface
    //    CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);

    //    // Set the path to the shortcut target and add the description
    //    hRes = ipShellLink->SetPath(szPath);
    //    if (FAILED(hRes))
    //        return hRes;

    //    hRes = ipShellLink->SetDescription(lpszDesc);
    //    if (FAILED(hRes))
    //        return hRes;

    //    // IPersistFile is using LPCOLESTR, so make sure
    //    // that the string is Unicode
    //#if !defined _UNICODE
    //    MultiByteToWideChar(CP_ACP, 0,
    //                   lpszShortcutPath, -1, wszTemp, MAX_PATH);
    //#else
    //    wcsncpy(wszTemp, lpszShortcutPath, MAX_PATH);
    //#endif

    //    // Write the shortcut to disk
    //    hRes = ipPersistFile->Save(wszTemp, TRUE);
    //}

    return hRes;
}


} // namespace UnicornUtils
