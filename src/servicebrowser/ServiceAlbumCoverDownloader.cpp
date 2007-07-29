//
// C++ Implementation: ServiceAlbumCoverDownloader
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "ServiceAlbumCoverDownloader.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"

#include <KStandardDirs>


#include <QDir>
#include <QImage>

using namespace Meta;


Meta::ServiceAlbumWithCover::ServiceAlbumWithCover( QString name )
    : ServiceAlbum( name )
    , m_hasFetchedCover( false )
    , m_isFetchingCover ( false )
    , m_coverDownloader( 0 )
{
}


Meta::ServiceAlbumWithCover::ServiceAlbumWithCover( QStringList resultRow )
    : ServiceAlbum( resultRow )
    , m_hasFetchedCover( false )
    , m_isFetchingCover ( false )
    , m_coverDownloader( 0 )
{
}


Meta::ServiceAlbumWithCover::~ServiceAlbumWithCover()
{
    delete m_coverDownloader;
}


QPixmap ServiceAlbumWithCover::image(int size, bool withShadow) const
{


    QString coverName = downloadPrefix() + '_' + albumArtist()->name() + '_' + name() + "_cover.png";


    QDir cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    if ( size <= 1 )
        size = AmarokConfig::coverPreviewSize();
    QString sizeKey = QString::number( size ) + '@';
    QImage img;


    if( cacheCoverDir.exists( sizeKey + coverName ) ) {
        debug() << "Image exists in cache" << endl;
        img = QImage( cacheCoverDir.filePath( sizeKey + coverName ) );
        return QPixmap::fromImage( img );
    }
    else if ( m_hasFetchedCover ) {

        img = m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        img.save( cacheCoverDir.filePath( sizeKey + coverName ), "PNG" );
        return QPixmap::fromImage( img );

    } else if ( !m_isFetchingCover ) {
        m_isFetchingCover = true;

        if ( m_coverDownloader == 0 )
            m_coverDownloader = new ServiceAlbumCoverDownloader();
        m_coverDownloader->downloadCover( this );

    }

    return Album::image( size, withShadow );

}



void ServiceAlbumWithCover::setImage( const QImage & image ) const
{
    m_cover = image;
    m_hasFetchedCover = true;
    m_isFetchingCover = false;
    notifyObservers();
}

void ServiceAlbumWithCover::imageDownloadCanceled() const
{
    m_hasFetchedCover = false;
    m_isFetchingCover = false;
}






ServiceAlbumCoverDownloader::ServiceAlbumCoverDownloader()
    : m_album( 0 )
        , m_albumDownloadJob( 0 )
{
    m_tempDir = new KTempDir();
    m_tempDir->setAutoRemove( false );
}

ServiceAlbumCoverDownloader::~ ServiceAlbumCoverDownloader()
{
    delete m_tempDir;
}


void ServiceAlbumCoverDownloader::downloadCover( const  ServiceAlbumWithCover * album )
{
    m_album = album;

    KUrl downloadUrl( album->coverUrl() );

    m_coverDownloadPath = m_tempDir->name() + downloadUrl.fileName();

    debug() << "Download Cover: " << downloadUrl.url() << " to: " << m_coverDownloadPath << endl;

    m_albumDownloadJob = KIO::file_copy( downloadUrl, KUrl( m_coverDownloadPath ), -1, true, false, false );

    connect( m_albumDownloadJob, SIGNAL( result( KJob* ) ), SLOT( coverDownloadComplete( KJob* ) ) );
    connect( m_albumDownloadJob, SIGNAL( canceled( KJob* ) ), SLOT( coverDownloadCanceled( KJob * ) ) );

}

void ServiceAlbumCoverDownloader::coverDownloadComplete( KJob * downloadJob )
{

    debug() << "cover download complete" << endl;

    if ( !downloadJob || !downloadJob->error() == 0 )
    {
        debug() << "error detected" << endl;
        //we could not download, so inform album
        m_album->imageDownloadCanceled();
        
        return ;
    }
    if ( downloadJob != m_albumDownloadJob )
        return ; //not the right job, so let's ignore it

    m_album->setImage( QImage( m_coverDownloadPath ) );

    downloadJob->deleteLater();

    m_tempDir->unlink();

}

void ServiceAlbumCoverDownloader::coverDownloadCanceled(KJob * downloadJob)
{
    
    debug() << "cover download cancelled" << endl;
    m_album->imageDownloadCanceled();
}







#include "ServiceAlbumCoverDownloader.moc"
                 