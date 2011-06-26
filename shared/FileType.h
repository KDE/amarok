/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 * Copyright (c) 2010 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2010 Christian Wagner <christian.wagner86@gmx.at>                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SHARED_FILETYPE_H
#define SHARED_FILETYPE_H

#include <QStringList>
#include <QString>

namespace Amarok
{

//New FileTypes must also be added to s_fileTypeStrings in FileType.cpp
enum FileType
{
    Unknown     =  0,
    Mp3         =  1,
    Ogg         =  2,
    Flac        =  3,
    Mp4         =  4,
    Wma         =  5,
    Aiff        =  6,
    Mpc         =  7,
    TrueAudio   =  8,
    Wav         =  9,
    WavPack     = 10,
    Mod         = 11,
    S3M         = 12,
    IT          = 13,
    XM          = 14
};


class FileTypeSupport
{
public:
    static QString toString( Amarok::FileType ft );
    static QStringList possibleFileTypes();
    static Amarok::FileType fileType( const QString& extension );
private:
    static QStringList s_fileTypeStrings;
};

}

#endif /* SHARED_FILETYPE_H */
