/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_LASTFMMETA_P_H
#define AMAROK_LASTFMMETA_P_H

#include "Debug.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "LastFmSettings.h"
#include "libUnicorn/TrackInfo.h"
#include "libUnicorn/WebService/Request.h"
#include "Meta.h"

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <KStandardDirs>

#include <QDir>
#include <QImage>
#include <QList>
#include <QPixmap>
#include <QStringList>



namespace LastFm
{

class Track::Private : public QObject
{
    Q_OBJECT

    public:
        Track *t;
        QString trackPath;
        QString lastFmUri;

        QPixmap albumArt;
        QString artist;
        QString album;
        QString track;
        int length;

        //not sure what these are for but they exist in the LastFmBundle
        QString albumUrl;
        QString artistUrl;
        QString trackUrl;

        Meta::ArtistPtr artistPtr;
        Meta::AlbumPtr albumPtr;
        Meta::GenrePtr genrePtr;
        Meta::ComposerPtr composerPtr;
        Meta::YearPtr yearPtr;

        Request *m_webServiceRequest;

    public:
        Private()
            : lastFmUri( 0 )
            , m_webServiceRequest( 0 )
        {
            artist = QString ( "Last.fm" );
        }

        ~Private()
        {
            if( m_webServiceRequest )
            {
                m_webServiceRequest->abort();
                m_webServiceRequest->deleteLater();
            }
        }

        void notifyObservers();

        void setTrackInfo( const TrackInfo &trackInfo )
        {
            if( m_webServiceRequest )
            {
                m_webServiceRequest->abort();
                m_webServiceRequest->deleteLater();
                m_webServiceRequest = 0;
            }

            artist = trackInfo.artist();
            album = trackInfo.album();
            track = trackInfo.track();
            length = trackInfo.duration();
            trackPath = trackInfo.path();

            // need to reset other items
            albumUrl = "";
            trackUrl = "";
            albumArt = QPixmap();

            notifyObservers();

            if( !trackInfo.isEmpty() )
            {
                TrackMetaDataRequest *tmdr = new TrackMetaDataRequest();
                m_webServiceRequest = tmdr;
                connect( m_webServiceRequest, SIGNAL( result( Request * ) ), this, SLOT( requestResult( Request * ) ) );
                m_webServiceRequest->setLanguage( The::settings().appLanguage() );
                tmdr->setTrack( trackInfo );
                m_webServiceRequest->start();
            }
        }

    public slots:
        void requestResult( Request *request )
        {
            m_webServiceRequest->deleteLater();
            m_webServiceRequest = 0;

            TrackMetaDataRequest *tmdr = static_cast<TrackMetaDataRequest *>( request );
            if( request->succeeded() )
            {
                albumUrl = tmdr->metaData().albumPageUrl();
                trackUrl = tmdr->metaData().trackPageUrl();

                // TODO: need a separate request to get artist url, it can wait until we're ready to do something with it

                notifyObservers();

                if( !tmdr->metaData().albumPicUrl().isEmpty() )
                {
                    KIO::Job* job = KIO::storedGet( KUrl( tmdr->metaData().albumPicUrl() ), KIO::Reload, KIO::HideProgressInfo );
                    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( fetchImageFinished( KJob* ) ) );
                }
            }
        }

        void fetchImageFinished( KJob* job )
        {
            if( job->error() == 0 ) {
                const int size = AmarokConfig::coverPreviewSize();

                QImage img = QImage::fromData( static_cast<KIO::StoredTransferJob*>( job )->data() );
                if( !img.isNull() )
                {
                    img.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

                    albumArt = QPixmap::fromImage( img );
                }
                else
                    albumArt = QPixmap();
            }
            else
            {
                //use default image
                albumArt = QPixmap();
            }
            notifyObservers();
        }
};

// internal helper classes

class LastFmArtist : public Meta::Artist
{
public:
    LastFmArtist( Track::Private *dptr )
        : Meta::Artist()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Meta::AlbumList albums()
    {
        return Meta::AlbumList();
    }

    QString name() const
    {
        if( d )
            return d->artist;
        else
            return QString( "Last.fm" );
    }

    QString prettyName() const
    {
        if( d )
            return d->artist;
        else
            return QString( "Last.fm" );
    }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmAlbum : public Meta::Album
{
public:
    LastFmAlbum( Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
    {}

    bool isCompilation() const { return false; }
    bool hasAlbumArtist() const { return false; }
    Meta::ArtistPtr albumArtist() const { return Meta::ArtistPtr(); }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d )
            return d->album;
        else
            return QString();
    }

    QString prettyName() const
    {
        if( d )
            return d->album;
        else
            return QString();
    }

    QPixmap image( int size, bool withShadow )
    {
        Q_UNUSED( withShadow )

        if( !d || d->albumArt.isNull() ) {
            //return Meta::Album::image( size, withShadow );
            //TODO implement shadow
            //TODO improve this

            if ( size <= 1 )
                size = AmarokConfig::coverPreviewSize();
            QString sizeKey = QString::number( size ) + '@';

            QImage img;
            QDir cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
            if( cacheCoverDir.exists( sizeKey + "lastfm-default-cover.png" ) )
                img = QImage( cacheCoverDir.filePath( sizeKey + "lastfm-default-cover.png" ) );
            else
            {
                QImage orgImage = QImage( KStandardDirs::locate( "data", "amarok/images/lastfm-default-cover.png" ) ); //optimise this!
                //scaled() does not change the original image but returns a scaled copy
                img = orgImage.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
                img.save( cacheCoverDir.filePath( sizeKey + "lastfm-default-cover.png" ), "PNG" );
            }

            m_noCoverImage = true;
            return QPixmap::fromImage( img );
        }
            
            
        if( d->albumArt.width() != size )
            return d->albumArt.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        else
            return d->albumArt;
    }

    // return true since we handle our own fetching
    bool hasImage( int size = 1 ) const { Q_UNUSED( size ); return true; }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmGenre : public Meta::Genre
{
public:
    LastFmGenre( Track::Private *dptr )
        : Meta::Genre()
        , d( dptr )
    {}

    QString name() const
    {
        return QString();
    }

    QString prettyName() const
    {
        return QString();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmComposer : public Meta::Composer
{
public:
    LastFmComposer( Track::Private *dptr )
        : Meta::Composer()
        , d( dptr )
    {}

    QString name() const
    {
        return QString();
    }

    QString prettyName() const
    {
        return QString();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmYear : public Meta::Year
{
public:
    LastFmYear( Track::Private *dptr )
        : Meta::Year()
        , d( dptr )
    {}

    QString name() const
    {
        return QString();
    }

    QString prettyName() const
    {
        return QString();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Track::Private * const d;

    friend class Track::Private;
};

void
Track::Private::notifyObservers()
{
    // TODO: only notify what actually has changed
    t->notifyObservers();
    static_cast<LastFmAlbum *>( t->album().data() )->notifyObservers();
    static_cast<LastFmArtist *>( t->artist().data() )->notifyObservers();
}

}

#endif
