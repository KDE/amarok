/*
    Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
    Copyright (C) 2008 Leo Franchi        <lfranchi@kde.org>

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
#include "meta/Meta.h"

#include <lastfm/ws/WsKeys.h>
#include <lastfm/types/Track.h>
#include <lastfm/ws/WsReply.h>
#include <lastfm/ws/WsRequestBuilder.h>
#include <lastfm/radio/Tuner.h>

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
        ::Track lastFmTrack; // this is how we love, ban, etc
        QUrl trackPath;
        QUrl lastFmUri;

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

    public:
        Private()
            : lastFmUri( QUrl() )
        {
            artist = QString ( "Last.fm" );
        }

        ~Private()
        {
        }

        void notifyObservers();

        void setTrackInfo( const ::Track &trackInfo )
        {
            lastFmTrack = trackInfo;
            artist = trackInfo.artist();
            album = trackInfo.album();
            track = trackInfo.title();
            length = trackInfo.duration();
            trackPath = trackInfo.url();
            
            // need to reset other items
            albumUrl = "";
            trackUrl = "";
            albumArt = QPixmap();

            notifyObservers();

            if( !trackInfo.isNull() )
            {
                WsReply* reply = WsRequestBuilder( "track.getInfo" )
                .add( "artist",artist)
                .add( "track", track )
                .add( "api_key", QString( Ws::ApiKey ) )
                .get();
                
                connect( reply, SIGNAL( finished( WsReply* ) ), SLOT( requestResult( WsReply* ) ) );
            }
        }

    public slots:
        void requestResult( WsReply *reply )
        {

            if( reply->error() == Ws::NoError )
            {
                albumUrl = reply->lfm()[ "track" ][ "album" ][ "url" ].text();
                trackUrl = reply->lfm()[ "track" ][ "url" ].text();
                artistUrl = reply->lfm()[ "track" ][ "artist" ][ "url" ].text();

                notifyObservers();

                if( !reply->lfm()[ "track" ][ "album" ][ "image size=large" ].text().isEmpty() )
                {
                    KIO::Job* job = KIO::storedGet( KUrl( reply->lfm()[ "track" ][ "album" ][ "image size=large" ].text() ), KIO::Reload, KIO::HideProgressInfo );
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
        return QString( "Last.fm" );
    }

    QString prettyName() const
    {
        if( d )
            return d->artist;
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
        return QString();
    }

    QString prettyName() const
    {
        if( d )
            return d->album;
        return QString();
    }

    QPixmap image( int size )
    {
        if( !d || d->albumArt.isNull() )
        {
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
            
            
        if( d->albumArt.width() != size && size > 0 )
            return d->albumArt.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
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
