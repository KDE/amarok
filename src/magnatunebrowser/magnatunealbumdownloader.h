/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef MAGNATUNEALBUMDOWNLOADER_H
#define MAGNATUNEALBUMDOWNLOADER_H

#include "magnatunedownloadinfo.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <qobject.h>

#include <ktempdir.h>
/**
This class encapsulates the downloading of an album once all required information has been retrieved

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneAlbumDownloader: public QObject
{
Q_OBJECT
public:
    MagnatuneAlbumDownloader();

    ~MagnatuneAlbumDownloader();

     void downloadCover( QString albumCoverUrlString, QString fileName );

signals:

    /**
     * This signal is emitted when a download is finished or cancelled
     * @param success true is download completed, false if download was cancelled.
     */
    void downloadComplete(bool success);
    void coverDownloadCompleted(QString coverFileName);

public slots:
    /**
     * Initiates the download of an album
     * @param url A MagnatuneDownloadInfo object containing all needed information
     */
    void downloadAlbum( MagnatuneDownloadInfo * info );

protected:

    KIO::FileCopyJob * m_albumDownloadJob;
    QString m_currentAlbumUnpackLocation;
    QString m_currentAlbumFileName;
    int m_currentAlbumId;
    KTempDir m_tempDir;

protected slots:
    /**
     * Unzip the downloaded album
     * @param downLoadJob 
     */
    void albumDownloadComplete( KIO::Job* downloadJob );
    void albumDownloadAborted();

    void coverDownloadComplete( KIO::Job* downloadJob );
    void coverDownloadAborted();

    void coverAddComplete( KIO::Job* downloadJob );
    void coverAddAborted();

};

#endif
