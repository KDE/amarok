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

#include <KLocalizedString>

using namespace Transcoding;

Mp3Format::Mp3Format()
{
    m_encoder = MP3;
    m_fileExtension = QStringLiteral("mp3");
    const QString description1 =
        i18n( "The bitrate is a measure of the quantity of data used to represent a "
        "second of the audio track.<br>The <b>MP3</b> encoder used by Amarok supports "
        "a <a href=http://en.wikipedia.org/wiki/MP3#VBR>variable bitrate (VBR)</a> "
        "setting, which means that the bitrate value fluctuates along the track "
        "based on the complexity of the audio content. More complex intervals of "
        "data are encoded with a higher bitrate than less complex ones; this "
        "approach yields overall better quality and a smaller file than having a "
        "constant bitrate throughout the track.<br>"
        "For this reason, the bitrate measure in this slider is just an estimate "
        "of the average bitrate of the encoded track.<br>"
        "<b>160kb/s</b> is a good choice for music listening on a portable player.<br/>"
        "Anything below <b>120kb/s</b> might be unsatisfactory for music and anything above "
        "<b>205kb/s</b> is probably overkill.");
    QStringList valueLabels;
    QByteArray vbr = "VBR ~%1kb/s";
    valueLabels
        << i18n( vbr, 80 )
        << i18n( vbr, 100 )
        << i18n( vbr, 120 )
        << i18n( vbr, 140 )
        << i18n( vbr, 160 )
        << i18n( vbr, 175 )
        << i18n( vbr, 190 )
        << i18n( vbr, 205 )
        << i18n( vbr, 220 )
        << i18n( vbr, 240 );

    m_propertyList << Property::Tradeoff( "quality", i18n( "Expected average bitrate for variable bitrate encoding" ), description1,
                                          i18n( "Smaller file" ), i18n( "Better sound quality" ),
                                          valueLabels, 5 );
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

QIcon
Mp3Format::icon() const
{
    return QIcon::fromTheme( QStringLiteral("audio-x-generic") );  //TODO: get a *real* icon!
}

QStringList
Mp3Format::ffmpegParameters( const Configuration &configuration ) const
{
    QStringList parameters;
    parameters << QStringLiteral("-acodec") << QStringLiteral("libmp3lame");
    for( const Property &property : m_propertyList )
    {
        if( !configuration.property( property.name() ).isNull()
            && configuration.property( property.name() ).type() == property.variantType() )
        {
            if( property.name() == "quality" )
            {
                int ffmpegQuality = qAbs( configuration.property( "quality" ).toInt() - 9 );
                parameters << QStringLiteral("-aq") << QString::number( ffmpegQuality );
            }
        }
    }
    parameters << QStringLiteral("-vcodec") << QStringLiteral("copy"); // keep album art unchanged
    return parameters;
}

bool
Mp3Format::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegularExpression( QStringLiteral("^ .EA... mp3 +.*libmp3lame") ) );
}
