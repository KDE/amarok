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

#include "core/support/Debug.h"
#include "core/transcoding/TranscodingController.h"

#include <KProcess>

#include <QFile>
#include <QTimer>

namespace Transcoding
{

Job::Job( const QUrl &src,
                            const QUrl &dest,
                            const Transcoding::Configuration &configuration,
                            QObject *parent )
    : KJob( parent )
    , m_src( src )
    , m_dest( dest )
    , m_configuration( configuration )
    , m_duration( -1 )
{
    init();
}

Job::Job( QUrl &src,
                            const Transcoding::Configuration &configuration,
                            QObject *parent )
    : KJob( parent )
    , m_src( src )
    , m_dest( src )
    , m_configuration( configuration )
    , m_duration( -1 )
{
    QString fileExtension = Amarok::Components::transcodingController()->format( configuration.encoder() )->fileExtension();
    if( !( fileExtension.isEmpty() ) )
    {
        QString destPath = src.path();
        destPath.truncate( destPath.lastIndexOf( QLatin1Char('.') ) + 1 );
        destPath.append( fileExtension );
        m_dest.setPath( destPath );
    }
    init();
}

void
Job::init()
{
    m_transcoder = new KProcess( this );

    m_transcoder->setOutputChannelMode( KProcess::MergedChannels );

    //First the executable...
    m_transcoder->setProgram( QStringLiteral("ffmpeg") );
    //... prevent ffmpeg from being interactive when destination file already exists. We
    //    would use -n to exit immediately, but libav's ffmpeg doesn't support it, so we
    //    check for destination file existence manually and pass -y (overwrite) to avoid
    //    race condition
    *m_transcoder << QStringLiteral( "-y" );
    //... then we'd have the infile configuration followed by "-i" and the infile path...
    *m_transcoder << QStringLiteral( "-i" )
                  << m_src.path();
    //... and finally, outfile configuration followed by the outfile path.
    const Transcoding::Format *format = Amarok::Components::transcodingController()->format( m_configuration.encoder() );
    *m_transcoder << format->ffmpegParameters( m_configuration )
                  << m_dest.path();

    connect( m_transcoder, &KProcess::readyRead,
             this, &Job::processOutput );
    connect( m_transcoder, QOverload<int, QProcess::ExitStatus>::of(&KProcess::finished),
             this, &Job::transcoderDone );
}

void
Job::start()
{
    DEBUG_BLOCK
    if( QFile::exists( m_dest.path() ) )
    {
        debug() << "Not starting ffmpeg encoder, file already exists:" << m_dest.path();
        QTimer::singleShot( 0, this, &Job::transcoderDoneDefault );
    }
    else
    {
        QString commandline = QStringLiteral( "'" ) + m_transcoder->program().join(QStringLiteral("' '")) + QStringLiteral( "'" );
        debug()<< "Calling" << commandline.toLocal8Bit().constData();
        m_transcoder->start();
    }
}

void
Job::transcoderDone( int exitCode, QProcess::ExitStatus exitStatus ) //SLOT
{
    if( exitCode == 0 && exitStatus == QProcess::NormalExit )
        debug() << "YAY, transcoding done!";
    else
    {
        debug() << "NAY, transcoding fail!";
        setError( KJob::UserDefinedError );
        setErrorText( QStringLiteral( "Calling `" ) + m_transcoder->program().join(QStringLiteral(" ")) + QStringLiteral("` failed") );
    }
    emitResult();
}

void
Job::transcoderDoneDefault()
{
    transcoderDone( -1, QProcess::CrashExit );
}

void
Job::processOutput()
{
    QString output = QString::fromLocal8Bit( m_transcoder->readAllStandardOutput().data() );
    if( output.simplified().isEmpty() )
        return;
    for( const QString &line : output.split( QLatin1Char( '\n' ) ) )
        debug() << "ffmpeg:" << line.toLocal8Bit().constData();

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
    QRegularExpression matchDuration( QStringLiteral( "Duration: (\\d{2,}):(\\d{2}):(\\d{2})\\.(\\d{2})" ) );
    QRegularExpressionMatch rmatch = matchDuration.match( output );

    if( rmatch.hasMatch() )
    {
        //duration is in csec
        qint64 duration = rmatch.captured( 1 ).toLong() * 60 * 60 * 100 +
                          rmatch.captured( 2 ).toInt()  * 60 * 100 +
                          rmatch.captured( 3 ).toInt()  * 100 +
                          rmatch.captured( 4 ).toInt();
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
    QRegularExpression matchTime( QStringLiteral("time=(\\d+)\\.(\\d{2})") );
    QRegularExpressionMatch rmatch = matchTime.match( output );

    if( rmatch.hasMatch() )
    {
        qint64 time = rmatch.captured( 1 ).toLong() * 100 +
                      rmatch.captured( 2 ).toInt();
        return time;
    }
    else
        return -1;
}

} //namespace Transcoding
