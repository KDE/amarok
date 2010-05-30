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
    DEBUG_BLOCK
    init();
}

TranscodeJob::TranscodeJob( KUrl &src, const TranscodeFormat &options, QObject *parent )
    : KJob( parent )
    , m_src( src )
    , m_dest( src )
    , m_options( options )
{
    DEBUG_BLOCK
    debug() << "TranscodeJob ctor!!";
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

    //First the executable...
    m_transcoder->setProgram( "ffmpeg" );
    //... then we'd have the infile options followed by "-i" and the infile path...
    *m_transcoder << QString( "-i" )
                  << m_src.path();
    //... and finally, outfile options followed by the outfile path.
    *m_transcoder << m_options.ffmpegParameters()
                  << m_dest.path();

    //debug spam follows
    debug() << "foo";
    debug() << m_options.ffmpegParameters();
    debug() << QString( "FFMPEG call is " ) << m_transcoder->program();

    connect( m_transcoder, SIGNAL( finished( int, QProcess::ExitStatus ) ),
             this, SLOT( transcoderDone( int, QProcess::ExitStatus ) ) );
}

void
TranscodeJob::start()
{
    DEBUG_BLOCK
    debug()<< "starting ffmpeg";
    debug()<< "call is " << m_transcoder->program();
    m_transcoder->start();
    debug() << m_transcoder->readAllStandardOutput();
    debug()<< "ffmpeg started";
}

void
TranscodeJob::transcoderDone( int exitCode, QProcess::ExitStatus exitStatus ) //SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( exitStatus );
    debug() << m_transcoder->readAll();
    if( !exitCode )
    {
        debug() << "YAY, transcoding done!";
        emitResult();
    }
    else
    {
        debug() << "NAY, transcoding fail!";
        emitResult();
    }
}
