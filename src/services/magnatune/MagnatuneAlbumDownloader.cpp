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
#include <KZip>

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

void
MagnatuneAlbumDownloader::downloadAlbum( MagnatuneDownloadInfo info )
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

    if( info.album() && info.album()->albumArtist() )
        The::statusBar()->newProgressOperation( m_albumDownloadJob, i18n( "Downloading '%1' by %2 from Magnatune.com", info.album()->prettyName(), info.album()->albumArtist()->prettyName() ) )->setAbortSlot( this, SLOT( albumDownloadAborted() ) );
    else
        The::statusBar()->newProgressOperation( m_albumDownloadJob, i18n( "Downloading album from Magnatune.com" ) )->setAbortSlot( this, SLOT( albumDownloadAborted() ) );
}




void
MagnatuneAlbumDownloader::albumDownloadComplete( KJob * downloadJob )
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

    const QString finalAlbumPath = m_currentAlbumUnpackLocation + '/' + m_currentAlbum->albumArtist()->name() + '/' + m_currentAlbum->name();

    //ok, now we have the .zip file downloaded. All we need is to unpack it to the desired location and add it to the collection.

    KZip kzip( m_tempDir->name() + m_currentAlbumFileName );

    if ( !kzip.open( QIODevice::ReadOnly ) )
    {
        The::statusBar()->shortMessage( i18n( "Magnatune download seems to have failed. Cannot read zip file" ) );
        emit( downloadComplete( false ) );
        return;
    }

    debug() << m_tempDir->name() + m_currentAlbumFileName << " opened for decompression";

    const KArchiveDirectory * directory = kzip.directory();

    The::statusBar()->shortMessage( i18n( "Uncompressing Magnatune.com download..." ) );

    //Is this really blocking with no progress status!? Why is it not a KJob?

    debug() <<  "decompressing to " << finalAlbumPath;
    directory->copyTo( m_currentAlbumUnpackLocation );

    debug() <<  "done!";
    

    if ( m_currentAlbum ) {

        //now I really want to add the album cover to the same folder where I just unzipped the album... The
        //only way of getting the actual location where the album was unpacked is using the artist and album names
        
        QString coverUrlString = m_currentAlbum->coverUrl();

        KUrl downloadUrl( coverUrlString.replace( "_200.jpg", "_1400.jpg") );

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

void
MagnatuneAlbumDownloader::albumDownloadAborted( )
{
    DEBUG_BLOCK
    
    The::statusBar()->endProgressOperation( m_albumDownloadJob );
    m_albumDownloadJob->kill();
    m_albumDownloadJob = 0;
    debug() << "Aborted album download";

    emit( downloadComplete( false ) );

}

#include "MagnatuneAlbumDownloader.moc"
