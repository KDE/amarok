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

#include "TranscodeFormat.h"
#include "core/support/Debug.h"

TranscodeFormat
TranscodeFormat::Null()
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::NULL_CODEC );
    debug() << "Constructed Null-codec.";
    return format;
}

TranscodeFormat
TranscodeFormat::Aac( int quality )
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::AAC );
    format.m_ffmpegParameters << "-aq" << QString::number( quality );
    debug()<< "In the named ctor, ffmpeg parameters are "<<format.m_ffmpegParameters;
    return format;
}

TranscodeFormat
TranscodeFormat::Alac()
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::ALAC );
    debug()<< "In the named ctor, ffmpeg parameters are "<<format.m_ffmpegParameters;
    return format;
}

TranscodeFormat
TranscodeFormat::Flac( int level )
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::FLAC );
    format.m_ffmpegParameters << "-aq" << QString::number( level );
    debug()<< "In the named ctor, ffmpeg parameters are "<<format.m_ffmpegParameters;
    return format;
}

TranscodeFormat
TranscodeFormat::Mp3( int v_rating )
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::MP3 );
    format.m_ffmpegParameters << "-aq" << QString::number( v_rating );
    debug()<< "In the named ctor, ffmpeg parameters are "<<format.m_ffmpegParameters;
    return format;
}

TranscodeFormat
TranscodeFormat::Vorbis( int quality )
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::VORBIS );
    format.m_ffmpegParameters << "-aq" << QString::number( quality );
    debug()<< "In the named ctor, ffmpeg parameters are "<<format.m_ffmpegParameters;
    return format;
}

TranscodeFormat
TranscodeFormat::Wma( int quality )
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::WMA2 );
    format.m_ffmpegParameters << "-aq" << QString::number( quality );
    debug()<< "In the named ctor, ffmpeg parameters are "<<format.m_ffmpegParameters;
    return format;
}

QStringList
TranscodeFormat::ffmpegParameters() const
{
    DEBUG_BLOCK
    if( !m_ffmpegParameters.isEmpty() )
    {
        debug() << "About to return ffmpeg parameters: " << m_ffmpegParameters;
        return m_ffmpegParameters;
    }
    debug() << "INVALID FFMPEG PARAMETERS";
    return QStringList();
}

QString
TranscodeFormat::fileExtension() const
{
    DEBUG_BLOCK
    switch( m_encoder )
    {
    case NULL_CODEC:
        return QString();
    case AAC:
        return QString( "m4a" );
    case ALAC:
        return QString( "m4a" );
    case FLAC:
        return QString( "flac" );
    case MP3:
        return QString( "mp3" );
    case VORBIS:
        return QString( "ogg" );
    case WMA2:
        return QString( "wma" );
    default:
        debug() << "Bad encoder.";
    }
    return QString();
}

QString
TranscodeFormat::prettyName( Encoder encoder )
{
    switch( encoder )
    {
    case NULL_CODEC:
        return QString();
    case AAC:
        return i18n( "AAC" );
    case ALAC:
        return i18n( "Apple Lossless" );
    case FLAC:
        return i18n( "FLAC" );
    case MP3:
        return i18n( "MP3" );
    case VORBIS:
        return i18n( "Ogg Vorbis" );
    case WMA2:
        return i18n( "Windows Media Audio" );
    default:
        debug() << "Bad encoder.";
    }
    return QString();
}

QString
TranscodeFormat::description( Encoder encoder ) //TODO!!!!!!!!!!!!!!
{
    switch( encoder )
    {
    case NULL_CODEC:
        return QString();
    case AAC:
        return i18n( "<a href=http://en.wikipedia.org/wiki/Advanced_Audio_Coding>Advanced Audio Coding</a> (AAC) is a patented lossy codec for digital audio.<br>AAC generally achieves better sound quality than MP3 at similar bit rates. It is the preferred lossy encoder for the iPod and some other portable music players." );
    case ALAC:
        return i18n( "<a href=http://en.wikipedia.org/wiki/Apple_Lossless>Apple Lossless</a> (ALAC) is an audio codec for lossless compression of digital music.<br>Recommended only for Apple music players and players that do not support FLAC." );
    case FLAC:
        return i18n( "<a href=http://en.wikipedia.org/wiki/Free_Lossless_Audio_Codec>Free Lossless Audio Codec</a> (FLAC) is an open and royalty-free codec for lossless compression of digital music.<br>If you wish to store your music without compromising on audio quality, FLAC is an excellent choice." );
    case MP3:
        return i18n( "<a href=http://en.wikipedia.org/wiki/MP3>MPEG Audio Layer 3</a> (MP3) is a patented digital audio codec using a form of lossy data compression.<br>In spite of its shortcomings, it is a common format for consumer audio storage, and is widely supported on portable music players." );
    case VORBIS:
        return i18n( "<a href=http://en.wikipedia.org/wiki/Vorbis>Ogg Vorbis</a> if an open and royalty-free audio codec for lossy audio compression.<br>It produces smaller files than MP3 at equivalent or higher quality. Ogg Vorbis is an all-around excellent choice, especially for portable music players that support it." );
    case WMA2:
        return i18n( "<a href=http://en.wikipedia.org/wiki/Windows_Media_Audio>Windows Media Audio</a> (WMA) is a proprietary codec developed by Microsoft for lossy audio compression.<br>Recommended only for portable music players that do not support Ogg Vorbis." );
    default:
        debug() << "Bad encoder.";
    }
    return QString();
}

KIcon
TranscodeFormat::icon( Encoder encoder ) //TODO!!!!!!!!!!!!!!
{
    switch( encoder )
    {
    case NULL_CODEC:
        return KIcon();
    case AAC:
        return KIcon( "audio-ac3" );
    case ALAC:
        return KIcon( "audio-x-flac" );
    case FLAC:
        return KIcon( "audio-x-flac" );
    case MP3:
        return KIcon( "audio-x-generic" );
    case VORBIS:
        return KIcon( "audio-x-wav" );
    case WMA2:
        return KIcon( "audio-x-generic" );
    default:
        debug() << "Bad encoder.";
    }
    return KIcon();
}

//private
TranscodeFormat::TranscodeFormat( Encoder encoder )
    : m_encoder( encoder )
{
    DEBUG_BLOCK
    switch( m_encoder )
    {
    case NULL_CODEC:
        m_ffmpegParameters.clear();
        break;
    case AAC:
        m_ffmpegParameters << "-acodec" << "libfaac";
        break;
    case ALAC:
        m_ffmpegParameters << "-acodec" << "alac";
        break;
    case FLAC:
        m_ffmpegParameters << "-acodec" << "flac";
        break;
    case MP3:
        m_ffmpegParameters << "-acodec" << "libmp3lame";
        break;
    case VORBIS:
        m_ffmpegParameters << "-acodec" << "libvorbis";
        break;
    case WMA2:
        m_ffmpegParameters << "-acodec" << "wmav2";
        break;
    default:
        debug() << "Bad encoder.";
    }

}
