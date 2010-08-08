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

#include "TranscodingVorbisFormat.h"

#include <KLocale>

namespace Transcoding
{

VorbisFormat::VorbisFormat()
{
    m_encoder = VORBIS;
    m_fileExtension = "ogg";
    m_propertyList << Property::Numeric( "quality", i18n( "Quality" ), 0, 10, 7 );
}

QString
VorbisFormat::prettyName() const
{
    return i18n( "Ogg Vorbis" );
}

QString
VorbisFormat::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/Vorbis>Ogg Vorbis</a> if an open "
                  "and royalty-free audio codec for lossy audio compression.<br>It produces "
                  "smaller files than MP3 at equivalent or higher quality. Ogg Vorbis is an "
                  "all-around excellent choice, especially for portable music players that "
                  "support it." );
}

KIcon
VorbisFormat::icon() const
{
    return KIcon( "audio-x-wav" );  //TODO: get a *real* icon!
}

QStringList
VorbisFormat::ffmpegParameters( const Configuration &configuration ) const
{
    return QStringList() << "-acodec" << "libvorbis";   //libvorbis is better than FFmpeg's
}                                                       //vorbis implementation in many ways

bool
VorbisFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegExp( ".EA... libvorbis" ) );
}

}
