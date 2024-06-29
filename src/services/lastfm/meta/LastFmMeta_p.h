/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_LASTFMMETA_P_H
#define AMAROK_LASTFMMETA_P_H

#include "core/support/Debug.h"

#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core-impl/support/TagStatisticsStore.h"

#include <KIO/Job>

#include <QDir>
#include <QImage>
#include <QList>
#include <QPixmap>
#include <QStandardPaths>
#include <QStringList>

#include <Track.h>
#include <ws.h>
#include <RadioTuner.h>
#include <XmlQuery.h>

namespace LastFm
{

class Track::Private : public QObject
{
    Q_OBJECT

    public:
        Track *t;
        lastfm::Track lastFmTrack; // this is how we love, ban, etc
        QUrl trackPath;
        QUrl lastFmUri;

        QImage albumArt;
        QString artist;
        QString album;
        QString track;
        qint64 length;

        //not sure what these are for but they exist in the LastFmBundle
        QString albumUrl;
        QString artistUrl;
        QString trackUrl;
        QString imageUrl;

        Meta::ArtistPtr artistPtr;
        Meta::AlbumPtr albumPtr;
        Meta::GenrePtr genrePtr;
        Meta::ComposerPtr composerPtr;
        Meta::YearPtr yearPtr;

        QNetworkReply* trackFetch;
        QNetworkReply* wsReply;

        Meta::StatisticsPtr statsStore;
        uint currentTrackStartTime;

    public:
        Private()
            : lastFmUri( QUrl() )
            , currentTrackStartTime( 0 )
        {
            artist = QString ( "Last.fm" );
        }

        ~Private() override
        {
        }

        void notifyObservers();

        void setTrackInfo( const lastfm::Track &trackInfo )
        {
            DEBUG_BLOCK
            bool newTrackInfo = artist != trackInfo.artist() ||
                                album != trackInfo.album() ||
                                track != trackInfo.title();


            lastFmTrack = trackInfo;
            artist = trackInfo.artist();
            album = trackInfo.album();
            track = trackInfo.title();
            length = trackInfo.duration() * 1000;
            trackPath = trackInfo.url();

            // need to reset other items
            albumUrl = "";
            trackUrl = "";
            albumArt = QImage();

            if( newTrackInfo )
            {
                statsStore = new TagStatisticsStore( t );
                currentTrackStartTime = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
            }

            notifyObservers();

            if( !trackInfo.isNull() )
            {
                QMap< QString, QString > params;
                params[ "method" ] = "track.getInfo";
                params[ "artist" ] = artist;
                params[ "track" ] = track;

                m_userFetch = lastfm::ws::post( params );

                connect( m_userFetch, SIGNAL( finished() ), SLOT( requestResult() ) );
            }
        }

    public Q_SLOTS:
        void requestResult( )
        {
            if( !m_userFetch )
                return;
            if( m_userFetch->error() == QNetworkReply::NoError )
            {
                lastfm::XmlQuery lfm;
                if( lfm.parse( m_userFetch->readAll() ) )
                {
                    albumUrl = lfm[ "track" ][ "album" ][ "url" ].text();
                    trackUrl = lfm[ "track" ][ "url" ].text();
                    artistUrl = lfm[ "track" ][ "artist" ][ "url" ].text();

                    notifyObservers();

                    imageUrl = lfm[ "track" ][ "album" ][ "image size=large" ].text();

                    if( !imageUrl.isEmpty() )
                    {
                        KIO::Job* job = KIO::storedGet( QUrl( imageUrl ), KIO::Reload, KIO::HideProgressInfo );
                        connect( job, SIGNAL( result( KJob* ) ), this, SLOT( fetchImageFinished( KJob* ) ) );
                    }
                }
                else
                {
                    debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
                    return;
                }
            }

        }

        void fetchImageFinished( KJob* job )
        {
            if( job->error() == 0 )
            {
                const int size = 100;

                QImage img = QImage::fromData( static_cast<KIO::StoredTransferJob*>( job )->data() );
                if( !img.isNull() )
                {
                    img.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

                    albumArt = img;
                }
                else
                    albumArt = QImage();
            }
            else
            {
                //use default image
                albumArt = QImage();
            }
            notifyObservers();
        }

    private:
        QNetworkReply* m_userFetch;

};

// internal helper classes

class LastFmArtist : public Meta::Artist
{
public:
    explicit LastFmArtist( Track::Private *dptr )
        : Meta::Artist()
        , d( dptr )
    {}

    Meta::TrackList tracks() override
    {
        return Meta::TrackList();
    }

    QString name() const override
    {
        if( d )
            return d->artist;
        return QStringLiteral( "Last.fm" );
    }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmAlbum : public Meta::Album
{
public:
    explicit LastFmAlbum( Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
    {}

    bool isCompilation() const override { return false; }
    bool hasAlbumArtist() const override { return false; }
    Meta::ArtistPtr albumArtist() const override { return Meta::ArtistPtr(); }

    Meta::TrackList tracks() override
    {
        return Meta::TrackList();
    }

    QString name() const override
    {
        if( d )
            return d->album;
        return QString();
    }

    QImage image( int size ) const override
    {
        if( !d || d->albumArt.isNull() )
        {
            //return Meta::Album::image( size, withShadow );
            //TODO implement shadow
            //TODO improve this

            if ( size <= 1 )
                size = 100;
            QString sizeKey = QString::number( size ) + QLatin1Char('@');

            QImage image;
            QDir cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
            if( cacheCoverDir.exists( sizeKey + "lastfm-default-cover.png" ) )
                image = QImage( cacheCoverDir.filePath( sizeKey + "lastfm-default-cover.png" ) );
            else
            {
                QImage orgImage = QImage( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/lastfm-default-cover.png" ) ); //optimize this!
                //scaled() does not change the original image but returns a scaled copy
                image = orgImage.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
                image.save( cacheCoverDir.filePath( sizeKey + "lastfm-default-cover.png" ), "PNG" );
            }

            return image;
        }


        if( d->albumArt.width() != size && size > 0 )
            return d->albumArt.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        return d->albumArt;
    }

    QUrl imageLocation( int size ) override
    {
        Q_UNUSED( size );
        if( d && !d->imageUrl.isEmpty() )
            return QUrl( d->imageUrl );
        return QUrl();
    }

    // return true since we handle our own fetching
    bool hasImage( int size = 1 ) const override { Q_UNUSED( size ); return true; }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmGenre : public Meta::Genre
{
public:
    explicit LastFmGenre( Track::Private *dptr )
        : Meta::Genre()
        , d( dptr )
    {}

    QString name() const override
    {
        return QString();
    }

    Meta::TrackList tracks() override
    {
        return Meta::TrackList();
    }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmComposer : public Meta::Composer
{
public:
    explicit LastFmComposer( Track::Private *dptr )
        : Meta::Composer()
        , d( dptr )
    {}

    QString name() const override
    {
        return QString();
    }

    Meta::TrackList tracks() override
    {
        return Meta::TrackList();
    }

    Track::Private * const d;

    friend class Track::Private;
};

class LastFmYear : public Meta::Year
{
public:
    explicit LastFmYear( Track::Private *dptr )
        : Meta::Year()
        , d( dptr )
    {}

    QString name() const override
    {
        return QString();
    }

    Meta::TrackList tracks() override
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
