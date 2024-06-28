/****************************************************************************************
 * Copyright (c) 2013 Martin Brodbeck <martin@brodbeck-online.de>                       *
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

#include "TranscodingOpusFormat.h"

#include <KLocalizedString>

using namespace Transcoding;

OpusFormat::OpusFormat()
{
    m_encoder = OPUS;
    m_fileExtension = QStringLiteral("opus");
    const QString description1 =
        i18n( "The bitrate is a measure of the quantity of data used to represent a "
        "second of the audio track.<br>The <b>Opus</b> encoder used by Amarok supports "
        "a <a href=http://en.wikipedia.org/wiki/Variable_bitrate>variable bitrate (VBR)</a> "
        "setting, which means that the bitrate value fluctuates along the track "
        "based on the complexity of the audio content. More complex intervals of "
        "data are encoded with a higher bitrate than less complex ones; this "
        "approach yields overall better quality and a smaller file than having a "
        "constant bitrate throughout the track.<br>"
        "For this reason, the bitrate measure in this slider is just an estimate "
        "of the average bitrate of the encoded track.<br>"
        "<b>128kb/s</b> is a good choice for music listening on a portable player.<br/>"
        "Anything below <b>100kb/s</b> might be unsatisfactory for music and anything above "
        "<b>256kb/s</b> is probably overkill.");
    QStringList valueLabels;
    char vbr[] = "VBR ~%1kb/s";
    valueLabels
        << i18n( vbr, 32 )
        << i18n( vbr, 64 )
        << i18n( vbr, 96 )
        << i18n( vbr, 128 )
        << i18n( vbr, 160 )
        << i18n( vbr, 192 )
        << i18n( vbr, 256 )
        << i18n( vbr, 320 )
        << i18n( vbr, 360 );
    m_validBitrates
        << 32
        << 64
        << 96
        << 128
        << 160
        << 192
        << 256
        << 320
        << 360;

    m_propertyList << Property::Tradeoff( "bitrate", i18n( "Expected average bitrate for variable bitrate encoding" ), description1,
                                          i18n( "Smaller file" ), i18n( "Better sound quality" ),
                                          valueLabels, 4 );
}

QString
OpusFormat::prettyName() const
{
    return i18n( "Opus" );
}

QString
OpusFormat::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/Opus_(audio_format)>Opus</a> is "
                  "a patent-free digital audio codec using a form of lossy data compression.");
}

QIcon
OpusFormat::icon() const
{
    return QIcon::fromTheme( QStringLiteral("audio-x-generic") );  //TODO: get a *real* icon!
}

QStringList
OpusFormat::ffmpegParameters( const Configuration &configuration ) const
{
    QStringList parameters;
    parameters << QStringLiteral("-acodec") << QStringLiteral("libopus");
    for( const Property &property : m_propertyList )
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
OpusFormat::toFfmpegBitrate( int setting ) const
{
    return m_validBitrates[ setting ] * 1000;
}

bool
OpusFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegularExpression( QStringLiteral("^ .EA... opus +.*libopus") ) );
}
