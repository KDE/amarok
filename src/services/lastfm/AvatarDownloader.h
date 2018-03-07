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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AVATAR_DOWNLOADER_H
#define AVATAR_DOWNLOADER_H

#include "network/NetworkAccessManagerProxy.h"

#include <QHash>
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
        * Start the download
        * @param url The url that should be downloaded.
        */
        void downloadAvatar( const QString& username, const QUrl &url );

    Q_SIGNALS:
        void avatarDownloaded( const QString &username, QPixmap avatar );

    private Q_SLOTS:
        /**
         * Slot called when the network access manager finished a request
         */
        void downloaded( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

    private:
        QHash<QUrl, QString> m_userAvatarUrls;
};

#endif
