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

#include <KLocalizedString>

using namespace Transcoding;

VorbisFormat::VorbisFormat()
{
    m_encoder = VORBIS;
    m_fileExtension = QStringLiteral("ogg");
    const QString description1 =
        i18n( "The bitrate is a measure of the quantity of data used to represent a "
        "second of the audio track.<br>The <b>Vorbis</b> encoder used by Amarok supports "
        "a <a href=http://en.wikipedia.org/wiki/Vorbis#Technical_details>variable bitrate "
        "(VBR)</a> setting, which means that the bitrate value fluctuates along the track "
        "based on the complexity of the audio content. More complex intervals of "
        "data are encoded with a higher bitrate than less complex ones; this "
        "approach yields overall better quality and a smaller file than having a "
        "constant bitrate throughout the track.<br>"
        "The Vorbis encoder uses a quality rating \"-q parameter\" between -1 and 10 to define "
        "a certain expected audio quality level. The bitrate measure in this slider is "
        "just a rough estimate (provided by Vorbis) of the average bitrate of the encoded "
        "track given a q value. In fact, with newer and more efficient Vorbis versions the "
        "actual bitrate is even lower.<br>"
        "<b>-q5</b> is a good choice for music listening on a portable player.<br/>"
        "Anything below <b>-q3</b> might be unsatisfactory for music and anything above "
        "<b>-q8</b> is probably overkill.");
    QStringList valueLabels;
    const char vbr[] = "-q%1 ~%2kb/s";
    valueLabels
        << i18n( vbr, -1, 45 )
        << i18n( vbr, 0, 64 )
        << i18n( vbr, 1, 80 )
        << i18n( vbr, 2, 96 )
        << i18n( vbr, 3, 112 )
        << i18n( vbr, 4, 128 )
        << i18n( vbr, 5, 160 )
        << i18n( vbr, 6, 192 )
        << i18n( vbr, 7, 224 )
        << i18n( vbr, 8, 256 )
        << i18n( vbr, 9, 320 )
        << i18n( vbr, 10, 500 );
    m_propertyList << Property::Tradeoff( "quality", i18n( "Quality rating for variable bitrate encoding" ), description1,
                                          i18n( "Smaller file" ), i18n( "Better sound quality" ),
                                          valueLabels, 7 );
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
                  "<a href=http://en.wikipedia.org/wiki/Vorbis>Ogg Vorbis</a> is an open "
                  "and royalty-free audio codec for lossy audio compression.<br>It produces "
                  "smaller files than MP3 at equivalent or higher quality. Ogg Vorbis is an "
                  "all-around excellent choice, especially for portable music players that "
                  "support it." );
}

QIcon
VorbisFormat::icon() const
{
    return QIcon::fromTheme( QStringLiteral("audio-x-wav") );  //TODO: get a *real* icon!
}

QStringList
VorbisFormat::ffmpegParameters( const Configuration &configuration ) const
{
    QStringList parameters;
    parameters << QStringLiteral("-acodec") << QStringLiteral("libvorbis");   //libvorbis is better than FFmpeg's
                                              //vorbis implementation in many ways
    for( const Property &property : m_propertyList )
    {
        if( !configuration.property( property.name() ).isNull()
            && configuration.property( property.name() ).type() == property.variantType() )
        {
            if( property.name() == "quality" )
            {
                int ffmpegQuality = configuration.property( "quality" ).toInt() - 1;
                parameters << QStringLiteral("-aq") << QString::number( ffmpegQuality );
            }
        }
    }
    parameters << QStringLiteral("-vn"); // no video stream or album art, some devices can't handle that
    return parameters;
}

bool
VorbisFormat::verifyAvailability( const QString &ffmpegOutput ) const
{
    return ffmpegOutput.contains( QRegularExpression( QStringLiteral("^ .EA... vorbis +.*libvorbis") ) );
}
