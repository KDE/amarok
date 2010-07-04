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

#ifndef TRANSCODEFORMAT_H
#define TRANSCODEFORMAT_H

#include "shared/amarok_export.h"

#include <QStringList>

class AMAROK_CORE_EXPORT TranscodeFormat
{
public:
    //WARNING: TranscodeFormat does *NOT* guarantee that the FFmpeg build on the target
    //system supports the specific encoder.
    enum Encoder
    {
        NULL_CODEC = 0, //null codec
        AAC,          //aac
        ALAC,         //alac
        FLAC,         //flac
        MP3,          //libmp3lame
        VORBIS,       //"libvorbis" supposedly gives better quality than "vorbis" but worse
                      //than oggenc. TODO: investigate
        WMA2          //wmav2  no idea what's this
    };

    //We make the real ctor private and use a bunch of named ctors because different codecs
    //take different parameters.
    static TranscodeFormat Null(); //don't transcode
    static TranscodeFormat Aac( int quality = 100 );//using libfaac - quality=percent of something (?) 100-150?
    static TranscodeFormat Alac();//http://forum.doom9.org/archive/index.php/t-140461.html - no params?
    static TranscodeFormat Flac( int level = 5 );//http://wiki.hydrogenaudio.org/index.php?title=Flac - just use level=5
    static TranscodeFormat Mp3( int v_rating ); //http://wiki.hydrogenaudio.org/index.php?title=LAME
    static TranscodeFormat Vorbis( int quality = 7 );
    static TranscodeFormat Wma( int quality );//wmav2, does it support vbr?

    QStringList ffmpegParameters() const;
    Encoder encoder() const { return m_encoder; }
    QString fileExtension() const;

private:
    explicit TranscodeFormat( Encoder encoder );
    Encoder m_encoder;
    QStringList m_ffmpegParameters;
};

#endif // TRANSCODEFORMAT_H
