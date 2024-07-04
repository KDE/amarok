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

#ifndef MAGNATUNEDOWNLOADHANDLER_H
#define MAGNATUNEDOWNLOADHANDLER_H

#include <QObject>
#include <kio/job.h>

#include "MagnatuneAlbumDownloader.h"
#include "MagnatuneDownloadDialog.h"
#include "MagnatuneMeta.h"


namespace KIO
{
    class TransferJob;
}

/**
The main class responsible for handling of downloads from Magnatune.com

@author Nikolaj Hald Nielsen
*/
class MagnatuneDownloadHandler : public QObject
{
Q_OBJECT
public:
    MagnatuneDownloadHandler();
    ~MagnatuneDownloadHandler() override;

   void setParent( QWidget * parent );
   /**
    * Starts a download operation
    * @param album The album to download
    */
   void downloadAlbum( Meta::MagnatuneAlbum * album );

Q_SIGNALS:

    void downloadCompleted( bool success );

private:
    KIO::TransferJob * m_resultDownloadJob;

    //need a parent to pass to any dialogs we spawn
    QWidget * m_parent;
    MagnatuneDownloadDialog * m_downloadDialog;
    MagnatuneAlbumDownloader * m_albumDownloader;
    Meta::MagnatuneAlbum * m_currentAlbum;
    QString m_currentAlbumCoverName;
    bool m_membershipDownload;

    bool parseDownloadXml( const QString &xml );
    void membershipDownload( int membershipType, const QString &username, const QString &password );

    /**
     * This function saves the xml download info received from Magnatune.com after a
     * successful download. This information can be used to later redownload a lost album,
     * or acquire an album in a different file format. Note that no personal information
     * or credit card number is stored. The information is saved to the amarok config
     * directory in the sub folder magnatune.com/purchases. The name of each info file
     * is generated from the artist and album names.
     * @param infoXml The info to store.
     */
    void saveDownloadInfo(const QByteArray &infoXml);


protected Q_SLOTS:

    void xmlDownloadComplete( KJob* downLoadJob );
    void albumDownloadComplete( bool success );

};

#endif
