/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "AvatarDownloader.h"

#include "Debug.h"
#include <QDir>
#include <QPixmap>


AvatarDownloader::AvatarDownloader()
: m_url( 0 )
, m_downloadPath()
, m_downloadJob( 0 )
, m_userName()
{
    m_tempDir = new KTempDir();
    m_tempDir->setAutoRemove( false );
}

AvatarDownloader::~AvatarDownloader()
{
    m_tempDir->unlink();
    delete m_tempDir;
}
QString
AvatarDownloader::username() const
{
    return m_userName;
}

void
AvatarDownloader::downloadAvatar(  const QString& username, const KUrl& url )
{
    m_userName = username;
    m_downloadPath = m_tempDir->name() + url.fileName();

//     debug() << "Download Avatar : " << url.url() << " to: " << m_downloadPath;

    m_downloadJob = KIO::file_copy( url, KUrl( m_downloadPath ), -1, KIO::Overwrite | KIO::HideProgressInfo );

    connect( m_downloadJob, SIGNAL( result( KJob* ) ), SLOT( downloadComplete( KJob* ) ) );
    connect( m_downloadJob, SIGNAL( canceled( KJob* ) ), SLOT( downloadCanceled( KJob * ) ) );
}

void
AvatarDownloader::downloadComplete( KJob * downloadJob )
{
    if( !downloadJob || !downloadJob->error() == 0 )
    {
        debug() << "Download Job failed!";
        return;
    }
    if ( downloadJob != m_downloadJob )
        return; //not the right job, so let's ignore it

    const QPixmap avatar = QPixmap( m_downloadPath );
    if ( avatar.isNull() )
    {
        debug() << "file not a valid image";
        return;
    }

    emit signalAvatarDownloaded( avatar );

    downloadJob->deleteLater();
}

void
AvatarDownloader::downloadCanceled( KJob *downloadJob )
{
    Q_UNUSED( downloadJob );

    debug() << "Avatar download cancelled";
}

#include "AvatarDownloader.moc"

