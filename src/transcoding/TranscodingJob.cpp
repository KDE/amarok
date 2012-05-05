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

#include "TranscodingJob.h"

#include "src/core/transcoding/TranscodingController.h"
#include "src/core/support/Debug.h"

#include <KProcess>

namespace Transcoding
{

Job::Job( const KUrl &src,
                            const KUrl &dest,
                            const Transcoding::Configuration &configuration,
                            QObject *parent )
    : KJob( parent )
    , m_src( src )
    , m_dest( dest )
    , m_configuration( configuration )
    , m_duration( -1 )
{
    DEBUG_BLOCK
    init();
}

Job::Job( KUrl &src,
                            const Transcoding::Configuration &configuration,
                            QObject *parent )
    : KJob( parent )
    , m_src( src )
    , m_dest( src )
    , m_configuration( configuration )
    , m_duration( -1 )
{
    DEBUG_BLOCK
    debug() << "Transcoding::Job ctor!!";
    debug()<< src;
    debug()<< src.path();
    QString fileExtension = Amarok::Components::transcodingController()->format( configuration.encoder() )->fileExtension();
    if( !( fileExtension.isEmpty() ) )
    {
        QString destPath = src.path();
        destPath.truncate( destPath.lastIndexOf( '.' ) + 1 );
        destPath.append( fileExtension );
        m_dest.setPath( destPath );
    }
    init();
}

void
Job::init()
{
    DEBUG_BLOCK
    m_transcoder = new KProcess( this );

    m_transcoder->setOutputChannelMode( KProcess::MergedChannels );

    //First the executable...
    m_transcoder->setProgram( "ffmpeg" );
    //... then we'd have the infile configuration followed by "-i" and the infile path...
    *m_transcoder << QString( "-i" )
                  << m_src.path();
    //... and finally, outfile configuration followed by the outfile path.
    const Transcoding::Format *format = Amarok::Components::transcodingController()->format( m_configuration.encoder() );
    *m_transcoder << format->ffmpegParameters( m_configuration )
                  << m_dest.path();

    //debug spam follows
    debug() << format->ffmpegParameters( m_configuration );
    debug() << QString( "FFMPEG call is " ) << m_transcoder->program().join(" ");

    connect( m_transcoder, SIGNAL( readyRead() ),
             this, SLOT( processOutput() ) );
    connect( m_transcoder, SIGNAL( finished( int, QProcess::ExitStatus ) ),
             this, SLOT( transcoderDone( int, QProcess::ExitStatus ) ) );
}

void
Job::start()
{
    DEBUG_BLOCK
    debug()<< "starting ffmpeg";
    debug()<< "call is " << m_transcoder->program().join(" ");
    m_transcoder->start();
    debug() << m_transcoder->readAllStandardOutput();
    debug()<< "ffmpeg started";
}

void
Job::transcoderDone( int exitCode, QProcess::ExitStatus exitStatus ) //SLOT
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

void
Job::processOutput()
{
    QString output = m_transcoder->readAllStandardOutput().data();
    if( output.simplified().isEmpty() )
        return;

    if( m_duration == -1 )
    {
        m_duration = computeDuration( output );
        if( m_duration >= 0 )
            setTotalAmount( KJob::Bytes, m_duration ); //Nothing better than bytes I can think of
    }

    qint64 progress = computeProgress( output  );
    if( progress > -1 )
        setProcessedAmount( KJob::Bytes, progress );
}

inline qint64
Job::computeDuration( const QString &output )
{
    //We match something like "Duration: 00:04:33.60"
    QRegExp matchDuration( "Duration: (\\d{2,}):(\\d{2}):(\\d{2})\\.(\\d{2})" );

    if( output.contains( matchDuration ) )
    {
        //duration is in csec
        qint64 duration = matchDuration.cap( 1 ).toLong() * 60 * 60 * 100 +
                          matchDuration.cap( 2 ).toInt()  * 60 * 100 +
                          matchDuration.cap( 3 ).toInt()  * 100 +
                          matchDuration.cap( 4 ).toInt();
        return duration;
    }
    else
        return -1;
}

inline qint64
Job::computeProgress( const QString &output )
{
    //Output is like size=     323kB time=18.10 bitrate= 146.0kbits/s
    //We're going to use the "time" column, which counts the elapsed time in seconds.
    QRegExp matchTime( "time=(\\d+)\\.(\\d{2})" );

    if( output.contains( matchTime ) )
    {
        qint64 time = matchTime.cap( 1 ).toLong() * 100 +
                      matchTime.cap( 2 ).toInt();
        return time;
    }
    else
        return -1;
}

} //namespace Transcoding
