/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/



#include "MagnatuneMeta.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"

#include <KStandardDirs>


#include <QDir>
#include <QImage>

using namespace Meta;

MagnatuneMetaFactory::MagnatuneMetaFactory(const QString & dbPrefix)
    : ServiceMetaFactory( dbPrefix )
{
}

int MagnatuneMetaFactory::getTrackSqlRowCount()
{
   return ServiceMetaFactory::getTrackSqlRowCount() + 1;
}

QString MagnatuneMetaFactory::getTrackSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getTrackSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_tracks.preview_lofi ";

    return sqlRows;
}


TrackPtr MagnatuneMetaFactory::createTrack(const QStringList & rows)
{
    return TrackPtr( new MagnatuneTrack( rows ) );
}

int MagnatuneMetaFactory::getAlbumSqlRowCount()
{
    return ServiceMetaFactory::getAlbumSqlRowCount() + 3;
}

QString MagnatuneMetaFactory::getAlbumSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getAlbumSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_albums.cover_url, ";
    sqlRows += tablePrefix() + "_albums.year, ";
    sqlRows += tablePrefix() + "_albums.album_code ";


    return sqlRows;
}

AlbumPtr MagnatuneMetaFactory::createAlbum(const QStringList & rows)
{
    return AlbumPtr( new MagnatuneAlbum( rows ) );
}

int MagnatuneMetaFactory::getArtistSqlRowCount()
{
    return ServiceMetaFactory::getArtistSqlRowCount() + 2;
}

QString MagnatuneMetaFactory::getArtistSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getArtistSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_artists.photo_url, ";
    sqlRows += tablePrefix() + "_artists.artist_page ";

    return sqlRows;
}

ArtistPtr MagnatuneMetaFactory::createArtist(const QStringList & rows)
{
    return ArtistPtr( new MagnatuneArtist( rows ) );
}


GenrePtr MagnatuneMetaFactory::createGenre(const QStringList & rows)
{
    return GenrePtr( new MagnatuneGenre( rows ) );
}


//// MagnatuneTrack ////

MagnatuneTrack::MagnatuneTrack( const QString &name )
    : ServiceTrack( name )
{
}

MagnatuneTrack::MagnatuneTrack(const QStringList & resultRow)
    : ServiceTrack( resultRow )
{
    DEBUG_BLOCK
    m_lofiUrl = resultRow[7];
}

QString MagnatuneTrack::lofiUrl()
{
    return m_lofiUrl;
}

void MagnatuneTrack::setLofiUrl(const QString & url)
{
    m_lofiUrl = url;
}


//// MagnatuneArtist ////

MagnatuneArtist::MagnatuneArtist( const QString &name )
    : ServiceArtist( name )
{
}

MagnatuneArtist::MagnatuneArtist(const QStringList & resultRow)
    : ServiceArtist( resultRow )
{
    m_photoUrl = resultRow[3];
    m_magnatuneUrl = resultRow[4];


}

void MagnatuneArtist::setPhotoUrl( const QString &photoUrl )
{
    m_photoUrl = photoUrl;
}

QString MagnatuneArtist::photoUrl( ) const
{
    return m_photoUrl;
}

void MagnatuneArtist::setMagnatuneUrl( const QString & magnatuneUrl )
{
    m_magnatuneUrl = magnatuneUrl;
}

QString MagnatuneArtist::magnatuneUrl() const
{
    return m_magnatuneUrl;
}






//// MagnatuneAlbum ////

MagnatuneAlbum::MagnatuneAlbum( const QString &name )
    : ServiceAlbum( name )
    , m_coverUrl()
    , m_launchYear( 0 )
    , m_albumCode()
    , m_hasFetchedCover( false )
    , m_isFetchingCover ( false )
    , m_coverDownloader( 0 )
{
}

MagnatuneAlbum::MagnatuneAlbum(const QStringList & resultRow)
    : ServiceAlbum( resultRow )
    , m_hasFetchedCover( false )
    , m_isFetchingCover ( false )
    , m_coverDownloader( 0 )
{
    debug() << "create album from result row: " << resultRow << endl;


    m_coverUrl = resultRow[4];
    m_launchYear = resultRow[5].toInt();
    m_albumCode = resultRow[6];

}

MagnatuneAlbum::~ MagnatuneAlbum()
{
    delete m_coverDownloader;
}


QString MagnatuneAlbum::name() const
{
    return ServiceAlbum::name();
}

void MagnatuneAlbum::setCoverUrl( const QString &coverUrl )
{
    m_coverUrl = coverUrl;
}

QString MagnatuneAlbum::coverUrl( ) const
{
    return m_coverUrl;
}

void MagnatuneAlbum::setLaunchYear( int launchYear )
{
    m_launchYear = launchYear;
}

int MagnatuneAlbum::launchYear( ) const
{
    return m_launchYear;
}

void MagnatuneAlbum::setAlbumCode(const QString & albumCode)
{
    m_albumCode = albumCode;
}

QString MagnatuneAlbum::albumCode()
{
    return m_albumCode;

}

QPixmap MagnatuneAlbum::image(int size, bool withShadow) const
{

    //TODO: some of this stuff should be factored out

    QString coverName = "magnatune_" + albumArtist()->name() + '_' + name() + "_cover.png";


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
           m_coverDownloader = new MagnatuneAlbumCoverDownloader();
           m_coverDownloader->downloadCover( this );

    }

    return Meta::Album::image( size, withShadow );

}

void MagnatuneAlbum::setImage( const QImage & image ) const
{
    m_cover = image;
    m_hasFetchedCover = true;
    m_isFetchingCover = false;
    notifyObservers();
}

void Meta::MagnatuneAlbum::imageDownloadCanceled() const
{
    m_hasFetchedCover = false;
    m_isFetchingCover = false;
}





MagnatuneGenre::MagnatuneGenre( const QString & name )
    : ServiceGenre( name )
{
}

MagnatuneGenre::MagnatuneGenre( const QStringList & resultRow )
    : ServiceGenre( resultRow )
{
}


MagnatuneAlbumCoverDownloader::MagnatuneAlbumCoverDownloader()
    : m_album( 0 )
    , m_albumDownloadJob( 0 )
{
    m_tempDir = new KTempDir();
    m_tempDir->setAutoRemove( false );
}

MagnatuneAlbumCoverDownloader::~ MagnatuneAlbumCoverDownloader()
{
    delete m_tempDir;
}


void MagnatuneAlbumCoverDownloader::downloadCover( const  MagnatuneAlbum * album )
{
    m_album = album;

    KUrl downloadUrl( album->coverUrl() );

    m_coverDownloadPath = m_tempDir->name() + downloadUrl.fileName();

    debug() << "Download Cover: " << downloadUrl.url() << " to: " << m_coverDownloadPath << endl;

    m_albumDownloadJob = KIO::file_copy( downloadUrl, KUrl( m_coverDownloadPath ), -1, true, false, false );

    connect( m_albumDownloadJob, SIGNAL( result( KJob* ) ), SLOT( coverDownloadComplete( KJob* ) ) );
    connect( m_albumDownloadJob, SIGNAL( canceled( KJob* ) ), SLOT( coverDownloadCanceled( KJob * ) ) );

}

void MagnatuneAlbumCoverDownloader::coverDownloadComplete( KJob * downloadJob )
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

void MagnatuneAlbumCoverDownloader::coverDownloadCanceled(KJob * downloadJob)
{
    
    debug() << "cover download cancelled" << endl;
}








#include "MagnatuneMeta.moc"
