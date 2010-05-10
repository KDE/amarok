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

TranscodeFormat &
TranscodeFormat::Vorbis( int quality )
{
    DEBUG_BLOCK
    TranscodeFormat format( TranscodeFormat::VORBIS );
    format.m_ffmpegParameters.append( " -aq " );
    format.m_ffmpegParameters.append( QString::number( quality ) );
    debug()<< "In the named ctor, ffmpeg parameters are "<<format.m_ffmpegParameters;
    return format;
}

//private
TranscodeFormat::TranscodeFormat( Encoder encoder )
    : m_encoder( encoder )
{
    m_ffmpegParameters = QString( "-acodec " );
    if( m_encoder == TranscodeFormat::VORBIS )
        m_ffmpegParameters.append( "libvorbis" );

}
