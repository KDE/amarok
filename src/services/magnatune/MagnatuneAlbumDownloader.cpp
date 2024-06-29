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

#include "MagnatuneAlbumDownloader.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/logger/Logger.h"
#include "MagnatuneMeta.h"

#include <QTemporaryDir>

#include <KLocalizedString>
#include <KZip>

MagnatuneAlbumDownloader::MagnatuneAlbumDownloader()
    : QObject()
    , m_albumDownloadJob( nullptr )
    , m_coverDownloadJob( nullptr )
    , m_currentAlbumFileName()
{
    m_tempDir = new QTemporaryDir();
}

MagnatuneAlbumDownloader::~MagnatuneAlbumDownloader()
{
    delete m_tempDir;
}

void
MagnatuneAlbumDownloader::downloadAlbum( MagnatuneDownloadInfo info )
{
    DEBUG_BLOCK

    m_currentAlbumInfo = info;


    QUrl downloadUrl = info.completeDownloadUrl();
    m_currentAlbumUnpackLocation = info.unpackLocation();
    debug() << "Download: " << downloadUrl.url() << " to: " << m_currentAlbumUnpackLocation;

    m_currentAlbumFileName = info.albumCode() + ".zip";

    debug() << "Using temporary location: " << m_tempDir->path() + QLatin1Char('/') + m_currentAlbumFileName;

    m_albumDownloadJob = KIO::file_copy( downloadUrl, QUrl::fromLocalFile( m_tempDir->path() + QLatin1Char('/') + m_currentAlbumFileName ), -1, KIO::Overwrite | KIO::HideProgressInfo );

    connect( m_albumDownloadJob, &KJob::result, this, &MagnatuneAlbumDownloader::albumDownloadComplete );

    QString msgText;
    if( !info.albumName().isEmpty() && !info.artistName().isEmpty() )
    {
        msgText = i18n( "Downloading '%1' by %2 from Magnatune.com", info.albumName(), info.artistName() );
    }
    else
    {
        msgText = i18n( "Downloading album from Magnatune.com" );
    }

    Amarok::Logger::newProgressOperation( m_albumDownloadJob, msgText, this, &MagnatuneAlbumDownloader::albumDownloadAborted );
}

void
MagnatuneAlbumDownloader::albumDownloadComplete( KJob * downloadJob )
{
    DEBUG_BLOCK

    debug() << "album download complete";

    if ( downloadJob->error() )
    {
        //TODO: error handling here
        return ;
    }
    if ( downloadJob != m_albumDownloadJob )
        return ; //not the right job, so let's ignore it

    const QString finalAlbumPath = m_currentAlbumUnpackLocation + QLatin1Char('/') + m_currentAlbumInfo.artistName() + QLatin1Char('/') + m_currentAlbumInfo.albumName();

    //ok, now we have the .zip file downloaded. All we need is to unpack it to the desired location and add it to the collection.

    KZip kzip( m_tempDir->path() + QLatin1Char('/') + m_currentAlbumFileName );

    if ( !kzip.open( QIODevice::ReadOnly ) )
    {
        Amarok::Logger::shortMessage( i18n( "Magnatune download seems to have failed. Cannot read zip file" ) );
        Q_EMIT( downloadComplete( false ) );
        return;
    }

    debug() << m_tempDir->path() + QLatin1Char('/') + m_currentAlbumFileName << " opened for decompression";

    const KArchiveDirectory * directory = kzip.directory();

    Amarok::Logger::shortMessage( i18n( "Uncompressing Magnatune.com download..." ) );

    //Is this really blocking with no progress status!? Why is it not a KJob?

    debug() <<  "decompressing to " << finalAlbumPath;
    directory->copyTo( m_currentAlbumUnpackLocation );

    debug() <<  "done!";
    


    //now I really want to add the album cover to the same folder where I just unzipped the album... The
    //only way of getting the actual location where the album was unpacked is using the artist and album names

    QString coverUrlString = m_currentAlbumInfo.coverUrl();

    QUrl downloadUrl( coverUrlString.replace( QStringLiteral("_200.jpg"), QStringLiteral(".jpg")) );

    debug() << "Adding cover " << downloadUrl.url() << " to collection at " << finalAlbumPath;

    m_coverDownloadJob = KIO::file_copy( downloadUrl, QUrl::fromLocalFile( finalAlbumPath + QStringLiteral("/cover.jpg") ), -1, KIO::Overwrite | KIO::HideProgressInfo );

    connect( m_coverDownloadJob, &KJob::result, this, &MagnatuneAlbumDownloader::coverDownloadComplete );

    Amarok::Logger::newProgressOperation( m_coverDownloadJob, i18n( "Adding album cover to collection" ), this, &MagnatuneAlbumDownloader::coverAddAborted );

    Q_EMIT( downloadComplete( true ) );
}

void
MagnatuneAlbumDownloader::coverDownloadComplete(KJob* downloadJob)
{
    DEBUG_BLOCK

    debug() << "cover download complete";

    if ( downloadJob->error() )
    {
        //TODO: error handling here
        return ;
    }
    if ( downloadJob != m_coverDownloadJob )
        return ; //not the right job, so let's ignore it

    //TODO: storing of cover here
}

void
MagnatuneAlbumDownloader::albumDownloadAborted( )
{
    DEBUG_BLOCK
    
    m_albumDownloadJob->kill();
    m_albumDownloadJob = nullptr;
    debug() << "Aborted album download";

    Q_EMIT( downloadComplete( false ) );
}

void
MagnatuneAlbumDownloader::coverAddAborted()
{
    DEBUG_BLOCK

    m_coverDownloadJob->kill();
    m_coverDownloadJob = nullptr;
    debug() << "Aborted cover download";

    Q_EMIT( downloadComplete( false ) );
}

