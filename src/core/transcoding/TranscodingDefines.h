/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef TRANSCODING_DEFINES_H
#define TRANSCODING_DEFINES_H

namespace Transcoding
{
    enum Encoder
    {
        INVALID,   // denotes invalid transcoding configuration
        JUST_COPY, // just copy or move the tracks (no transcoding)
        AAC,       // Advanced Audio Coding
        ALAC,      // Apple Lossless Audio Codec
        FLAC,      // Free Lossless Audio Codec
        MP3,       // MPEG-1 or MPEG-2 Audio Layer III encoded using lame encoder
        OPUS,      // Opus
        VORBIS,    // Ogg Vorbis
        WMA2       // Windows Media Audio 2
    };

}

#endif //TRANSCODING_DEFINES_H
