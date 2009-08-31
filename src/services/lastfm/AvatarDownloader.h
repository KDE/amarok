/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AVATAR_DOWNLOADER_H
#define AVATAR_DOWNLOADER_H

#include <KUrl>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <KTempDir>

#include <QObject>
#include <QPixmap>

class AvatarDownloader : public QObject
{
    Q_OBJECT

    public:

        /**
        * Constructor.
        */
        AvatarDownloader();

        /**
        * Destructor.
        */
        ~AvatarDownloader();

        /**
         * Get the username associated with this avatar
         */
        QString username() const;
        /**
        * Start the download
        * @param url The url that should be downloaded.
        */
        void downloadAvatar( const QString& username, const KUrl& url );
    signals:
        void signalAvatarDownloaded( QPixmap avatar );
    private slots:

        /**
        * Slot called when the download job is complete.
        * @param downloadJob The job responsible for the cover download.
        */
        void downloadComplete( KJob * downloadJob );

        /**
        * Slot called if the download job is cancelled.
        * @param downloadJob The job responsible for the cover download.
        */
        void downloadCanceled( KJob * downloadJob );

    private:
        KUrl * m_url;
        QString m_downloadPath;
        KIO::FileCopyJob * m_downloadJob;
        KTempDir * m_tempDir;
        QString m_userName;
};
#endif
