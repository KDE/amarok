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

#include "TranscodingMp3Format.h"

#include <KLocale>

namespace Transcoding
{

Mp3Format::Mp3Format()
{
    m_encoder = MP3;
    m_fileExtension = "mp3";
    QString description1 =
            i18n( "");
    m_propertyList << Property::Numeric( "quality", i18n( "Quality" ), description1, 9, 0, 6 );
}

QString
Mp3Format::prettyName() const
{
    return i18n( "MP3" );
}

QString
Mp3Format::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/MP3>MPEG Audio Layer 3</a> (MP3) is "
                  "a patented digital audio codec using a form of lossy data compression."
                  "<br>In spite of its shortcomings, it is a common format for consumer "
                  "audio storage, and is widely supported on portable music players." );
}

KIcon
Mp3Format::icon() const
{
    return KIcon( "audio-x-generic" );  //TODO: get a *real* icon!
}

QStringList
Mp3Format::ffmpegParameters( const Configuration &configuration ) const
{
    return QStringList() << "-acodec" << "libmp3lame";
}

bool
Mp3Format::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegExp( ".EA... libmp3lame" ) );
}

}
