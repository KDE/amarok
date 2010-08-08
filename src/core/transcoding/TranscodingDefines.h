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
    NULL_CODEC = 0, //null codec
    AAC,          //aac (would libfaac be better?)
    ALAC,         //alac
    FLAC,         //flac
    MP3,          //libmp3lame
    VORBIS,       //"libvorbis" supposedly gives better quality than "vorbis" but worse
                  //than oggenc. TODO: investigate
    WMA2,          //wmav2  no idea what's this
    NUM_CODECS
};

}

#endif //TRANSCODING_DEFINES_H
