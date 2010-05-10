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

#include "TranscodeJob.h"
#include "src/core/support/Debug.h"

#include <KProcess>

TranscodeJob::TranscodeJob( const KUrl &src, const KUrl &dest, const TranscodeFormat &options, QObject *parent )
    : KJob( parent )
    , m_src( src )
    , m_dest( dest )
    , m_options( options )
{
    init();
}

TranscodeJob::TranscodeJob( const KUrl &src, const TranscodeFormat &options, QObject *parent )
    : KJob( parent )
    , m_src( src )
    , m_dest( src )
    , m_options( options )
{
    DEBUG_BLOCK
    debug()<< src;
    debug()<< src.path();
    QString destPath = src.path();
    destPath.truncate( destPath.lastIndexOf( '.' ) + 1 );

    //what follows is a really really really bad way to distinguish between codecs
    if( m_options.encoder() == TranscodeFormat::MP3 )
        destPath.append( "mp3" );
    else if( m_options.encoder() == TranscodeFormat::FLAC )
        destPath.append( "flac" );
    else if( m_options.encoder() == TranscodeFormat::VORBIS )
        destPath.append( "ogg" );
    else
        destPath.append("mp3");    // Fallback to mp3 with ffmpeg's crappy default parameters.
                                   // You have been warned.
    m_dest.setPath( destPath );
    init();
}

void
TranscodeJob::init()
{
    DEBUG_BLOCK
    m_transcoder = new KProcess( this );

    m_transcoder->setOutputChannelMode( KProcess::MergedChannels );

    m_transcoder->setProgram( "ffmpeg" );
    *m_transcoder << "-i" << m_src.path() << m_dest.path();
    *m_transcoder << m_options.ffmpegParameters();
    debug() << "foo";
    debug() << m_options.ffmpegParameters();
    debug() << QString( "FFMPEG call is " ) << m_transcoder->program();
    connect( m_transcoder, SIGNAL( finished( int, QProcess::ExitStatus ) ),
             this, SLOT( transcoderDone( int, QProcess::ExitStatus ) ) );
}

void
TranscodeJob::start()
{
    debug()<< "starting ffmpeg";
    m_transcoder->start();
}

void
TranscodeJob::transcoderDone( int exitCode, QProcess::ExitStatus exitStatus ) //SLOT
{
    Q_UNUSED( exitStatus );
    if( !exitCode )
    {
        debug() << "YAY, transcoding done!";
    }
    emitResult();
}
