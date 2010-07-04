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

#ifndef TRANSCODEJOB_H
#define TRANSCODEJOB_H

#include "core/transcoding/TranscodeFormat.h"

#include <KJob>
#include <KUrl>

#include <KProcess>

/**
 * A KJob that transcodes an audio stream from a file into another file with a different
 * codec and container format.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_EXPORT TranscodeJob : public KJob
{
    Q_OBJECT
public:
    /**
     * Constructor. Creates a TranscodeJob and fills in the source, destination and
     * encoder parameters. The job does not start automatically.
     * @param src the path of the source file.
     * @param dest the path of the destination file, to be created.
     * @param options the string of parameters to be fed to the encoder. This implementation
     *        uses the FFmpeg executable, @see http://ffmpeg.org/ffmpeg-doc.html#SEC6
     * @param the parent QObject.
     */
    explicit TranscodeJob( const KUrl &src, const KUrl &dest, const TranscodeFormat &options, QObject *parent = 0 );

    /**
     * Convenience constructor. Creates a TranscodeJob with the destination file to be
     * placed in the same directory as the source.
     */
    explicit TranscodeJob( KUrl &src, const TranscodeFormat &options, QObject *parent = 0 );

    /**
     * Sets the path of the source file.
     * @param src the path of the source file.
     */
    void setSource( const KUrl &src );

    /**
     * Sets the path of the destination file, to be created.
     * @param dest the path of the destination file.
     */
    void setDestination( const KUrl &dest );

    /**
     * Starts the transcoding job.
     */
    void start();
/*
signals:
    void percent( KJob *job, unsigned long percent );
*/
private slots:
    void transcoderDone( int exitCode, QProcess::ExitStatus exitStatus );
    void init();

private:
    KUrl m_src;
    KUrl m_dest;
    TranscodeFormat m_options;
    KProcess *m_transcoder;
};

#endif // TRANSCODEJOB_H
