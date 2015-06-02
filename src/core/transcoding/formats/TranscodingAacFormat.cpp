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

#include <KLocale>

#include <QVariant>

using namespace Transcoding;

AacFormat::AacFormat()
{
    m_encoder = AAC;
    m_fileExtension = "m4a";
    QString description1 =
        i18n( "The bitrate is a measure of the quantity of data used to represent a second "
              "of the audio track.<br>The <b>AAC</b> encoder used by Amarok supports a <a href="
              "http://en.wikipedia.org/wiki/Variable_bitrate#Advantages_and_disadvantages_of_VBR"
              ">variable bitrate (VBR)</a> setting, which means that the bitrate value "
              "fluctuates along the track based on the complexity of the audio content. "
              "More complex intervals of data are encoded with a higher bitrate than less "
              "complex ones; this approach yields overall better quality and a smaller file "
              "than having a constant bitrate throughout the track.<br>"
              "For this reason, the bitrate measure in this slider is just an estimate "
              "of the <a href=http://www.ffmpeg.org/faq.html#SEC21>average bitrate</a> of "
              "the encoded track.<br>"
              "<b>150kb/s</b> is a good choice for music listening on a portable player.<br/>"
              "Anything below <b>120kb/s</b> might be unsatisfactory for music and anything above "
              "<b>200kb/s</b> is probably overkill." );
    QStringList valueLabels;
    QByteArray vbr = "VBR ~%1kb/s";
    valueLabels
        << i18n( vbr, 25 )
        << i18n( vbr, 50 )
        << i18n( vbr, 70 )
        << i18n( vbr, 90 )
        << i18n( vbr, 120 )
        << i18n( vbr, 150 )
        << i18n( vbr, 170 )
        << i18n( vbr, 180 )
        << i18n( vbr, 190 )
        << i18n( vbr, 200 )
        << i18n( vbr, 210 );
    m_propertyList << Property::Tradeoff( "quality", i18n( "Expected average bitrate for variable bitrate encoding" ), description1,
                                          i18n( "Smaller file" ), i18n( "Better sound quality"),
                                          valueLabels, 6 );
}

QString
AacFormat::prettyName() const
{
    return i18n( "AAC (Non-Free)" );
}

QString
AacFormat::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/Advanced_Audio_Coding>Advanced Audio "
                  "Coding</a> (AAC) is a patented lossy codec for digital audio.<br>AAC "
                  "generally achieves better sound quality than MP3 at similar bit rates. "
                  "It is a reasonable choice for the iPod and some other portable music "
                  "players. Non-Free implementation." );

}

QIcon
AacFormat::icon() const
{
    return QIcon::fromTheme( "audio-ac3" ); //TODO: get a *real* icon!
}

QStringList
AacFormat::ffmpegParameters( const Configuration &configuration ) const
{
    QStringList parameters;
    parameters << "-acodec" << "libfaac"; /* libfaac seems to be the only decent AAC encoder
                                             for GNU/Linux and it's a proprietary freeware
                                             with LGPL portions. Hopefully in the future
                                             FFmpeg's native aac implementation should get
                                             better so libfaac won't be necessary any more.
                                                            -- Teo 5/aug/2010 */
    foreach( const Property &property, m_propertyList )
    {
        if( !configuration.property( property.name() ).isNull()
            && configuration.property( property.name() ).type() == property.variantType() )
        {
            if( property.name() == "quality" )
            {
                parameters << "-aq"
                           << QString::number( configuration.property( "quality" ).toInt() * 25 + 5 );
            }
        }
    }
    return parameters;
}

bool
AacFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegExp( "^ .EA....*libfaac" ) );
}
