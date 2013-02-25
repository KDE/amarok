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

#include "amarokshared_export.h"

#include <QStringList>
#include <QString>

namespace Amarok
{
    //New FileTypes must also be added to s_fileTypeStrings in FileType.cpp
    enum FileType
    {
        Unknown     =  0,
        Mp3         =  1,
        Ogg         =  2, // please use just for Ogg Vorbis
        Flac        =  3,
        Mp4         =  4, // a file in MPEG-4 container that may or may not contain video
        Wma         =  5,
        Aiff        =  6,
        Mpc         =  7,
        TrueAudio   =  8,
        Wav         =  9,
        WavPack     = 10,
        M4a         = 11, // a file in MPEG-4 container that contains only audio
        M4v         = 12, // a file in MPEG-4 container that for sure contains video
        Mod         = 13,
        S3M         = 14,
        IT          = 15,
        XM          = 16,
        Speex       = 17,
        Opus        = 18
    };

    class AMAROKSHARED_EXPORT FileTypeSupport
    {
        public:
            /**
             * Return preferred extension of given filetype
             *
             * TODO: rename to extension()
             */
            static QString toString( Amarok::FileType ft );

            /**
             * Return a list of possible localized filetype strings.
             *
             * TODO: rename to possibleExtensions()
             */
            static QStringList possibleFileTypes();

            /**
             * Return file type given file extension, which must exclude the leading dot.
             *
             * @return Amarok::FileType enum, Amarok::Unknown if no other suitable
             *         type is in the enum
             */
            static Amarok::FileType fileType( const QString& extension );
    };
}

#endif /* SHARED_FILETYPE_H */
