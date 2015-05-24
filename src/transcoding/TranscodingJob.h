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

#ifndef TRANSCODING_JOB_H
#define TRANSCODING_JOB_H

#include "amarok_transcoding_export.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <KJob>
#include <QUrl>

#include <KProcess>

namespace Transcoding
{

/**
 * A KJob that transcodes an audio stream from a file into another file with a different
 * codec and container format.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_TRANSCODING_EXPORT Job : public KJob
{
    Q_OBJECT
public:
    /**
     * Constructor. Creates a Transcoding::Job and fills in the source, destination and
     * encoder parameters. The job does not start automatically.
     * @param src the path of the source file.
     * @param dest the path of the destination file, to be created.
     * @param configuration the string of parameters to be fed to the encoder. This implementation
     *        uses the FFmpeg executable, @see http://ffmpeg.org/ffmpeg-doc.html#SEC6
     * @param the parent QObject.
     */
    explicit Job( const QUrl &src,
                  const QUrl &dest,
                  const Transcoding::Configuration &configuration,
                  QObject *parent = 0 );

    /**
     * Convenience constructor. Creates a Transcoding::Job with the destination file to be
     * placed in the same directory as the source.
     */
    explicit Job( QUrl &src,
                  const Transcoding::Configuration &configuration,
                  QObject *parent = 0 );

    /**
     * Sets the path of the source file.
     * @param src the path of the source file.
     */
    void setSource( const QUrl &src );

    /**
     * Sets the path of the destination file, to be created.
     * @param dest the path of the destination file.
     */
    void setDestination( const QUrl &dest );

    /**
     * Starts the transcoding job.
     */
    void start();

    /**
     * Get the source url.
     */
    QUrl srcUrl() const { return m_src; }

    /**
     * Get the destination url.
     */
    QUrl destUrl() const { return m_dest; }

private slots:
    void processOutput();
    /**
     * Default arguments are for convenience (read: lazyness) so that this can be
     * connected to QTimer::singleShot()
     */
    void transcoderDone( int exitCode = -1, QProcess::ExitStatus exitStatus = QProcess::CrashExit );
    void init();

private:
    inline qint64 computeDuration( const QString &output );
    inline qint64 computeProgress( const QString &output );
    QUrl m_src;
    QUrl m_dest;
    Transcoding::Configuration m_configuration;
    KProcess *m_transcoder;
    qint64 m_duration; //in csec
};

} //namespace Transcoding

#endif //TRANSCODING_JOB_H
