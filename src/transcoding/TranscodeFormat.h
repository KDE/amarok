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

#include <QStringList>

class TranscodeFormat
{
public:
    enum Encoder
    {
        AAC = 0,      //aac
        ALAC,         //alac
        FLAC,         //flac
        MP3,          //libmp3lame
        VORBIS,       //"libvorbis" supposedly gives better quality than "vorbis" but worse
                      //than oggenc. TODO: investigate
        WMA2          //wmav2  no idea what's this
    };

    //We make the real ctor private and use a bunch of named ctors because different codecs
    //take different parameters.
    static TranscodeFormat Aac();
    static TranscodeFormat Alac();
    static TranscodeFormat Flac();
    static TranscodeFormat Mp3();
    static TranscodeFormat Vorbis( int quality );
    static TranscodeFormat Wma();

    QStringList ffmpegParameters() const;
    Encoder encoder() const { return m_encoder; }

private:
    explicit TranscodeFormat( Encoder encoder );
    Encoder m_encoder;
    QStringList m_ffmpegParameters;
};

#endif // TRANSCODEFORMAT_H
