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

#include "UnicornCommonMac.h"

#include <QDir>
#include <QString>
#include <Carbon/Carbon.h>

namespace UnicornUtils
{


QString
applicationSupportFolderPath()
{
    OSErr err;

    short vRefNum = 0;
    StrFileName fileName;
    fileName[0] = 0;
    long dirId;
    err = ::FindFolder( kOnAppropriateDisk, kApplicationSupportFolderType,
                        kDontCreateFolder, &vRefNum, &dirId );
    if ( err != noErr )
        return "";

    // Now we have a vRefNum and a dirID - but *not* an Unix-Path as string.
    // Lets make one based from this:

    // create a FSSpec...
    FSSpec fsspec;
    err = ::FSMakeFSSpec( vRefNum, dirId, NULL, &fsspec );
    if ( err != noErr )
        return "";

    // ...and build an FSRef based on thes FSSpec.
    FSRef fsref;
    err = ::FSpMakeFSRef( &fsspec, &fsref );
    if ( err != noErr )
        return "";

    // ...then extract the Unix Path as a C-String from the FSRef
    unsigned char path[512];
    err = ::FSRefMakePath( &fsref, path, 512 );
    if ( err != noErr )
        return "";

    return QDir::homePath() + QString::fromUtf8(reinterpret_cast<char *>(path));
}


QLocale::Language
osxLanguageCode()
{
    CFArrayRef languages;
    languages = (CFArrayRef)CFPreferencesCopyValue( CFSTR( "AppleLanguages" ),
                                                    kCFPreferencesAnyApplication,
                                                    kCFPreferencesCurrentUser,
                                                    kCFPreferencesAnyHost );

    QString langCode;

    if ( languages != NULL )
    {
        CFStringRef uxstylelangs = CFStringCreateByCombiningStrings( kCFAllocatorDefault, languages, CFSTR( ":" ) );
        langCode = UnicornUtils::CFStringToQString( uxstylelangs ).split( ':' ).value( 0 );
    }

    return QLocale( langCode ).language();
}

} // namespace UnicornUtils
