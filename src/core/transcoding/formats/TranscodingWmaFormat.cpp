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

#include <KLocalizedString>

using namespace Transcoding;

WmaFormat::WmaFormat()
{
    m_encoder = WMA2;
    m_fileExtension = QStringLiteral("wma");
    const QString description1 =
        i18n( "The bitrate is a measure of the quantity of data used to represent a "
        "second of the audio track.<br>Due to the limitations of the proprietary <b>WMA</b> "
        "format and the difficulty of reverse-engineering a proprietary encoder, the "
        "WMA encoder used by Amarok sets a <a href=http://en.wikipedia.org/wiki/"
        "Windows_Media_Audio#Windows_Media_Audio>constant bitrate (CBR)</a> setting.<br>"
        "For this reason, the bitrate measure in this slider is a pretty accurate estimate "
        "of the bitrate of the encoded track.<br>"
        "<b>136kb/s</b> is a good choice for music listening on a portable player.<br/>"
        "Anything below <b>112kb/s</b> might be unsatisfactory for music and anything above "
        "<b>182kb/s</b> is probably overkill.");
    QStringList valueLabels;
    const QByteArray cbr = "CBR %1kb/s";
    valueLabels
        << i18n( cbr, 64 )
        << i18n( cbr, 80 )
        << i18n( cbr, 96 )
        << i18n( cbr, 112 )
        << i18n( cbr, 136 )
        << i18n( cbr, 182 )
        << i18n( cbr, 275 )
        << i18n( cbr, 550 );
    m_validBitrates
        << 65
        << 75
        << 88
        << 106
        << 133
        << 180
        << 271
        << 545;

    m_propertyList << Property::Tradeoff( "bitrate", i18n( "Bitrate target for constant bitrate encoding" ), description1,
                                          i18n( "Smaller file" ), i18n( "Better sound quality" ),
                                          valueLabels, 4 );
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

QIcon
WmaFormat::icon() const
{
    return QIcon::fromTheme( "audio-x-generic" );  //TODO: get a *real* icon!
}

QStringList
WmaFormat::ffmpegParameters( const Configuration &configuration ) const
{
    QStringList parameters;
    parameters << QStringLiteral("-acodec") << QStringLiteral("wmav2");
    foreach( const Property &property, m_propertyList )
    {
        if( !configuration.property( property.name() ).isNull()
            && configuration.property( property.name() ).type() == property.variantType() )
        {
            if( property.name() == "bitrate" )
            {
                int ffmpegBitrate = toFfmpegBitrate( configuration.property( "bitrate" ).toInt() );
                parameters << QStringLiteral("-ab") << QString::number( ffmpegBitrate );
            }
        }
    }
    parameters << QStringLiteral("-vn"); // no video stream or album art
    return parameters;
}

int
WmaFormat::toFfmpegBitrate( int setting ) const
{
    return m_validBitrates[ setting ] * 1000;
}


bool
WmaFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegularExpression( QStringLiteral("^ .EA... wmav2 +") ) );
}
