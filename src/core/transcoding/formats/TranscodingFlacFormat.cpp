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

#include "TranscodingFlacFormat.h"

#include <KLocalizedString>

#include <QVariant>

using namespace Transcoding;

FlacFormat::FlacFormat()
{
    m_encoder = FLAC;
    m_fileExtension = QStringLiteral("flac");
    const QString description1 =
            i18n( "The <a href=http://flac.sourceforge.net/documentation_tools_flac.html>"
            "compression level</a> is an integer value between 0 and 8 that represents "
            "the tradeoff between file size and compression speed while encoding with <b>FLAC</b>.<br/> "
            "Setting the compression level to <b>0</b> yields the shortest compression time but "
            "generates a comparably big file<br/>"
            "On the other hand, a compression level of <b>8</b> makes compression quite slow but "
            "produces the smallest file.<br/>"
            "Note that since FLAC is by definition a lossless codec, the audio quality "
            "of the output is exactly the same regardless of the compression level.<br/>"
            "Also, levels above <b>5</b> dramatically increase compression time but create an only "
            "slightly smaller file, and are not recommended." );
    m_propertyList << Property::Tradeoff( "level", i18n( "Compression level" ), description1,
                                          i18n( "Faster compression" ), i18n( "Smaller file" ),
                                          0, 8, 5 );
}

QString
FlacFormat::prettyName() const
{
    return i18n( "FLAC" );
}

QString
FlacFormat::description() const
{
    return i18nc( "Feel free to redirect the english Wikipedia link to a local version, if "
                  "it exists.",
                  "<a href=http://en.wikipedia.org/wiki/Free_Lossless_Audio_Codec>Free "
                  "Lossless Audio Codec</a> (FLAC) is an open and royalty-free codec for "
                  "lossless compression of digital music.<br>If you wish to store your music "
                  "without compromising on audio quality, FLAC is an excellent choice." );
}

QIcon
FlacFormat::icon() const
{
    return QIcon::fromTheme( QStringLiteral("audio-x-flac") ); //TODO: get a *real* icon!
}

QStringList
FlacFormat::ffmpegParameters( const Configuration &configuration ) const
{
    QStringList parameters;
    parameters << QStringLiteral("-acodec") << QStringLiteral("flac");
    for( const Property &property : m_propertyList )
    {
        if( !configuration.property( property.name() ).isNull()
            && configuration.property( property.name() ).type() == property.variantType() )
        {
            if( property.name() == "level" )
            {
                parameters << QStringLiteral("-compression_level")
                           << QString::number( configuration.property( "level" ).toInt() );
            }
        }
    }
    parameters << QStringLiteral("-vn"); // no album art, writing it to flac is not supported by ffmpeg
    return parameters;
}

bool
FlacFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegularExpression( QStringLiteral("^ .EA... flac +") ) );
}
