/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef MAGNATUNEALBUMDOWNLOADER_H
#define MAGNATUNEALBUMDOWNLOADER_H

#include "MagnatuneDownloadInfo.h"
#include "MagnatuneMeta.h"

#include <KIO/Job>
#include <KIO/FileCopyJob>

#include <QObject>
#include <QTemporaryDir>

/**
This class encapsulates the downloading of an album once all required information has been retrieved

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MagnatuneAlbumDownloader : public QObject
{
Q_OBJECT
public:
    MagnatuneAlbumDownloader();

    ~MagnatuneAlbumDownloader() override;

Q_SIGNALS:

    /**
     * This signal is emitted when a download is finished or cancelled
     * @param success true is download completed, false if download was cancelled.
     */
    void downloadComplete( bool success );

public Q_SLOTS:
    /**
     * Initiates the download of an album
     * @param info A MagnatuneDownloadInfo object containing all needed information
     */
    void downloadAlbum( MagnatuneDownloadInfo info );

protected:

    KIO::FileCopyJob *m_albumDownloadJob;
    KIO::FileCopyJob *m_coverDownloadJob;
    QString m_currentAlbumUnpackLocation;
    QString m_currentAlbumFileName;
    MagnatuneDownloadInfo m_currentAlbumInfo;
    QTemporaryDir * m_tempDir;

protected Q_SLOTS:
    /**
     * Unzip the downloaded album
     * @param downloadJob
     */
    void albumDownloadComplete( KJob* downloadJob );
    void albumDownloadAborted();
    void coverDownloadComplete( KJob* downloadJob );
    void coverAddAborted();

};

#endif
