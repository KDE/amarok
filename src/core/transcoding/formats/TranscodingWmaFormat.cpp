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

#include "TranscodingWmaFormat.h"

#include <KLocale>

namespace Transcoding
{

WmaFormat::WmaFormat()
{
    m_encoder = WMA2;
    m_fileExtension = "wma";
    m_propertyList << Property::Numeric( "quality", i18n( "Quality" ), 0, 10, 7 );    //check docs
}

QString
WmaFormat::prettyName() const
{
    return i18n( "Windows Media Audio" );
}

QString
WmaFormat::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/Windows_Media_Audio>Windows Media "
                  "Audio</a> (WMA) is a proprietary codec developed by Microsoft for lossy "
                  "audio compression.<br>Recommended only for portable music players that "
                  "do not support Ogg Vorbis." );
}

KIcon
WmaFormat::icon() const
{
    return KIcon( "audio-x-generic" );  //TODO: get a *real* icon!
}

QStringList
WmaFormat::ffmpegParameters( const Configuration &configuration ) const
{
    return QStringList() << "-acodec" << "wmav2";
}

bool
WmaFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegExp( ".EA... wmav2" ) );
}

}
