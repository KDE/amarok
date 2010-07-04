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
        break;
    case AAC:
        return QString( "m4a" );
        break;
    case ALAC:
        return QString( "m4a" );
        break;
    case FLAC:
        return QString( "flac" );
        break;
    case MP3:
        return QString( "mp3" );
        break;
    case VORBIS:
        return QString( "ogg" );
        break;
    case WMA2:
        return QString( "wma" );
        break;
    default:
        debug() << "Bad encoder.";
        return QString();
    }
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
