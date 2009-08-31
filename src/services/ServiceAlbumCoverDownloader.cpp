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

#include "ServiceAlbumCoverDownloader.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"

#include <QDir>
#include <QImage>

using namespace Meta;


Meta::ServiceAlbumWithCover::ServiceAlbumWithCover( const QString &name )
    : ServiceAlbum( name )
    , m_cover( 0 )
    , m_hasFetchedCover( false )
    , m_isFetchingCover ( false )
    , m_coverDownloader( 0 )
{}

Meta::ServiceAlbumWithCover::ServiceAlbumWithCover( const QStringList &resultRow )
    : ServiceAlbum( resultRow )
    , m_cover( 0 )
    , m_hasFetchedCover( false )
    , m_isFetchingCover ( false )
    , m_coverDownloader( 0 )
{}

Meta::ServiceAlbumWithCover::~ServiceAlbumWithCover()
{
    delete m_coverDownloader;
    delete m_cover;
}

QPixmap
ServiceAlbumWithCover::image( int size )
{
    if( size > 1000 )
    {
        debug() << "Giant image detected, are you sure you want this?";
        return Meta::Album::image( size );
    }

    QString artist;

    if ( hasAlbumArtist() )
        artist = albumArtist()->name();
    else
        artist = "NULL"; //no need to translate, only used as a caching key/temp filename

    QString coverName = downloadPrefix() + '_' + artist+ '_' + name() + "_cover.png";

    QDir cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );

    //make sure that this dir exists
    if ( !cacheCoverDir.exists() )
        cacheCoverDir.mkpath( Amarok::saveLocation( "albumcovers/cache/" ) );

    if ( size <= 1 )
        size = 100;
    QString sizeKey = QString::number( size ) + '@';

    QPixmap pixmap;

    if( QFile::exists( cacheCoverDir.filePath( sizeKey + coverName ) ) )
    {
        pixmap = QPixmap( cacheCoverDir.filePath( sizeKey + coverName ) );
        return pixmap;
    }
    else if ( m_hasFetchedCover && m_cover && !m_cover->isNull() )
    {
        pixmap = m_cover->scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        pixmap.save( cacheCoverDir.filePath( sizeKey + coverName ), "PNG" );
        return pixmap;

    }
    else if ( !m_isFetchingCover && !coverUrl().isEmpty() )
    {
        m_isFetchingCover = true;

        if ( m_coverDownloader == 0 )
            m_coverDownloader = new ServiceAlbumCoverDownloader();
        m_coverDownloader->downloadCover( this );

    }

    return Album::image( size );
}

void
ServiceAlbumWithCover::setImage( const QPixmap& pixmap )
{
    delete m_cover;
    m_cover = new QPixmap( pixmap );
    m_hasFetchedCover = true;
    m_isFetchingCover = false;

    notifyObservers();
}

void
ServiceAlbumWithCover::imageDownloadCanceled() const
{
    m_hasFetchedCover = true;
    m_isFetchingCover = false;
}


///////////////////////////////////////////////////////////////////////////////
// Class ServiceAlbumCoverDownloader
///////////////////////////////////////////////////////////////////////////////

ServiceAlbumCoverDownloader::ServiceAlbumCoverDownloader()
    : m_album( 0 )
    , m_albumDownloadJob( 0 )
{
    m_tempDir = new KTempDir();
    m_tempDir->setAutoRemove( false );
}

ServiceAlbumCoverDownloader::~ServiceAlbumCoverDownloader()
{
    delete m_tempDir;
}

void
ServiceAlbumCoverDownloader::downloadCover( ServiceAlbumWithCover * album )
{
    m_album = album;

    KUrl downloadUrl( album->coverUrl() );

    m_coverDownloadPath = m_tempDir->name() + downloadUrl.fileName();

    debug() << "Download Cover: " << downloadUrl.url() << " to: " << m_coverDownloadPath;

    m_albumDownloadJob = KIO::file_copy( downloadUrl, KUrl( m_coverDownloadPath ), -1, KIO::Overwrite | KIO::HideProgressInfo );

    connect( m_albumDownloadJob, SIGNAL( result( KJob* ) ), SLOT( coverDownloadComplete( KJob* ) ) );
    connect( m_albumDownloadJob, SIGNAL( canceled( KJob* ) ), SLOT( coverDownloadCanceled( KJob * ) ) );
}

void
ServiceAlbumCoverDownloader::coverDownloadComplete( KJob * downloadJob )
{


    if( !downloadJob || !downloadJob->error() == 0 )
    {
        debug() << "Download Job failed!";

        //we could not download, so inform album
        if( m_album )
            m_album->imageDownloadCanceled();

        return;
    }

    if ( downloadJob != m_albumDownloadJob )
        return; //not the right job, so let's ignore it

    const QPixmap cover = QPixmap( m_coverDownloadPath );
    if ( cover.isNull() )
    {
        debug() << "file not a valid image";
        //the file wasn't an image, so inform album
        m_album->imageDownloadCanceled();

        return;
    }

    m_album->setImage( cover );

    downloadJob->deleteLater();

    m_tempDir->unlink();
}

void
ServiceAlbumCoverDownloader::coverDownloadCanceled( KJob *downloadJob )
{
    Q_UNUSED( downloadJob );
    DEBUG_BLOCK

    debug() << "Cover download cancelled";
    m_album->imageDownloadCanceled();
}

#include "ServiceAlbumCoverDownloader.moc"

