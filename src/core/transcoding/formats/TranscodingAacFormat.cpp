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

#include "TranscodingAacFormat.h"

#include <KLocalizedString>

#include <QVariant>

using namespace Transcoding;

AacFormat::AacFormat()
{
    m_encoder = AAC;
    m_fileExtension = QStringLiteral("m4a");
    QString description1 =
        i18n( "The bitrate is a measure of the quantity of data used to represent a "
              "second of the audio track.<br>"
              "The encoder used by Amarok operates better with a constant bitrate.<br>"
              "VBR is experimental and likely to get even worse results than the CBR.<br>"
              "For this reason, the bitrate measure in this slider is a pretty accurate estimate "
              "of the bitrate of the encoded track.<br>"
              "The encoder is transparent at 128kbps for most samples tested with artifacts only appearing in extreme cases.");
    QStringList valueLabels;
    QByteArray cbr = "CBR %1kb/s";
    valueLabels
        << i18n( cbr, 32 )
        << i18n( cbr, 64 )
        << i18n( cbr, 96 )
        << i18n( cbr, 128 )
        << i18n( cbr, 160 )
        << i18n( cbr, 192 )
        << i18n( cbr, 224 )
        << i18n( cbr, 256 );
    m_propertyList << Property::Tradeoff( "bitrate", i18n( "Bitrate target for constant bitrate encoding" ), description1,
                                          i18n( "Smaller file" ), i18n( "Better sound quality"),
                                          valueLabels, 3 );
}

QString
AacFormat::prettyName() const
{
    return i18n( "AAC" );
}

QString
AacFormat::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/Advanced_Audio_Coding>Advanced Audio "
                  "Coding</a> (AAC) is a patented lossy codec for digital audio.<br>AAC "
                  "generally achieves better sound quality than MP3 at similar bit rates. "
                  "It is a reasonable choice for the iPod and some other portable music players." );
}

QIcon
AacFormat::icon() const
{
    return QIcon::fromTheme( QStringLiteral("audio-ac3") ); //TODO: get a *real* icon!
}

QStringList
AacFormat::ffmpegParameters( const Configuration &configuration ) const
{
    QStringList parameters;
    parameters << QStringLiteral("-acodec") << QStringLiteral("aac") << QStringLiteral("-strict") << QStringLiteral("-2");
    foreach( const Property &property, m_propertyList )
    {
        if( !configuration.property( property.name() ).isNull()
            && configuration.property( property.name() ).type() == property.variantType() )
        {
            if( property.name() == "bitrate" )
            {
                parameters << QStringLiteral("-b:a")
                           << QString::number( ( configuration.property( "bitrate" ).toInt() + 1 ) * 32000);
            }
        }
    }
    parameters << QStringLiteral("-vn"); // no album art, writing it to m4a is not supported by ffmpeg
    return parameters;
}

bool
AacFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegularExpression( "^ .EA... aac +" ) );
}
