/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
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

#include "Amarok.h"
#include "Debug.h"
#include "MagnatuneMeta.h"
#include "statusbar/StatusBar.h"

#include <KLocale>
#include <kshell.h>

MagnatuneAlbumDownloader::MagnatuneAlbumDownloader()
    : QObject()
    , m_albumDownloadJob( 0 )
    , m_currentAlbumFileName()
    , m_currentAlbum( 0 )
{
    m_tempDir = new KTempDir();
    m_tempDir->setAutoRemove( false );

}


MagnatuneAlbumDownloader::~MagnatuneAlbumDownloader()
{
    delete m_tempDir;
    m_tempDir = 0;
}

void MagnatuneAlbumDownloader::downloadAlbum( MagnatuneDownloadInfo info )
{
    DEBUG_BLOCK

    m_currentAlbum = info.album();

    KUrl downloadUrl = info.completeDownloadUrl();
    m_currentAlbumUnpackLocation = info.unpackLocation();
    debug() << "Download: " << downloadUrl.url() << " to: " << m_currentAlbumUnpackLocation;


    //m_currentAlbumFileName = downloadUrl.fileName();
    if ( m_currentAlbum )
        m_currentAlbumFileName = m_currentAlbum->albumCode() + ".zip";
    else
        m_currentAlbumFileName = "temp_album.zip";


    debug() << "Using temporary location: " << m_tempDir->name() + m_currentAlbumFileName;

    m_albumDownloadJob = KIO::file_copy( downloadUrl, KUrl( m_tempDir->name() + m_currentAlbumFileName ), -1, KIO::Overwrite | KIO::HideProgressInfo );

    connect( m_albumDownloadJob, SIGNAL( result( KJob* ) ), SLOT( albumDownloadComplete( KJob* ) ) );

    The::statusBar()->newProgressOperation( m_albumDownloadJob, i18n( "Downloading album" ) )
    ->setAbortSlot( this, SLOT( albumDownloadAborted() ) );
}




void MagnatuneAlbumDownloader::albumDownloadComplete( KJob * downloadJob )
{
    DEBUG_BLOCK

    debug() << "album download complete";

    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( downloadJob != m_albumDownloadJob )
        return ; //not the right job, so let's ignore it

    //ok, now we have the .zip file downloaded. All we need is to unpack it to the desired location and add it to the collection.

    QString unzipString = "unzip " + KShell::quoteArg( m_tempDir->name() + m_currentAlbumFileName ) + " -d " + KShell::quoteArg( m_currentAlbumUnpackLocation ) + " &";

    debug() << "unpacking: " << unzipString;

    if ( system( unzipString.toAscii() ) == -1 )
        return;

    if ( m_currentAlbum ) {

        //now I really want to add the album cover to the same folder where I just unzipped the album... The
        //only way of getting the actual location where the album was unpacked is using the artist and album names

        QString finalAlbumPath = m_currentAlbumUnpackLocation + '/' + m_currentAlbum->albumArtist()->name() + '/' + m_currentAlbum->name();
        QString coverUrlString = m_currentAlbum->coverUrl();


        KUrl downloadUrl( coverUrlString );

        debug() << "Adding cover " << downloadUrl.url() << " to collection at " << finalAlbumPath;

        m_albumDownloadJob = KIO::file_copy( downloadUrl, KUrl( finalAlbumPath + "/cover.jpg" ), -1, KIO::Overwrite | KIO::HideProgressInfo );

        connect( m_albumDownloadJob, SIGNAL( result( KJob* ) ), SLOT( coverAddComplete( KJob* ) ) );

        The::statusBar()->newProgressOperation( m_albumDownloadJob, i18n( "Adding album cover to collection" ) )
        ->setAbortSlot( this, SLOT( coverAddAborted() ) );

        emit( downloadComplete( true ) );

    } else {

        //we do not know exactly what album this is (we are most likely using the redownload manager)
        emit( downloadComplete( true ) );
    }

}


void MagnatuneAlbumDownloader::albumDownloadAborted( )
{
    DEBUG_BLOCK
    
    The::statusBar()->endProgressOperation( m_albumDownloadJob );
    m_albumDownloadJob->kill();
    delete m_albumDownloadJob;
    m_albumDownloadJob = 0;
    debug() << "Aborted album download";

    emit( downloadComplete( false ) );

}


#include "MagnatuneAlbumDownloader.moc"





