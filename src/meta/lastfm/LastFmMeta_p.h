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

#include "debug.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "lastfm.h"
#include "meta.h"

#include <QImage>
#include <QList>
#include <QPixmap>
#include <QStringList>

#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace LastFm;

class Track::Private : public QObject
{
    Q_OBJECT

    public:
        Track *t;
        KUrl proxyUrl;
        QString lastFmUri;

        QList<Meta::TrackObserver*> observers;
        WebService *service;

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
        void notifyObservers()
        {
            foreach( Meta::TrackObserver *observer, observers )
                observer->metadataChanged( t );
        }

    public slots:
        void metaDataFinished( int /* id */, bool error )
        {
            DEBUG_BLOCK

            AmarokHttp* http = (AmarokHttp*) sender();
            http->deleteLater();
            if( error ) return;

            const QString result( http->readAll() );
            debug() << result << endl;

            int errCode = service->parameter( "error", result ).toInt();
            if ( errCode > 0 ) {
                debug() << "Metadata failed with error code: " << errCode << endl;
                service->showError( errCode );
                return;
            }
            artist = service->parameter( "artist", result );
            album = service->parameter( "album", result );
            track = service->parameter( "track", result );
            length = service->parameter( "trackduration", result ).toInt();
            artistUrl = service->parameter( "artist_url", result );
            albumUrl = service->parameter( "album_url", result );
            trackUrl = service->parameter( "track_url", result );

            QString imageUrl = service->parameter( "albumcover_medium", result );
            if( imageUrl == "http://static.last.fm/coverart/" ||
                imageUrl == "http://static.last.fm/depth/catalogue/no_album_large.gif" )
            {
                //no image available, get default image.
                albumArt = QPixmap();
                notifyObservers();
                return;
            }
            KIO::Job* job = KIO::storedGet( KUrl( imageUrl ), true, false );
            connect( job, SIGNAL( result( KJob* ) ), this, SLOT( fetchImageFinished( KJob* ) ) );
        }

        void fetchImageFinished( KJob* job )
        {
            if( job->error() == 0 ) {
                //do we still need to save the image to disk??
                const QString path = Amarok::saveLocation() + "lastfm_image.png";
                const int size = AmarokConfig::coverPreviewSize();

                QImage img = QImage::fromData( static_cast<KIO::StoredTransferJob*>( job )->data() );
                if( !img.isNull() )
                {
                    img.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ).save( path, "PNG" );

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

    QString name() const
    {
        if( d )
            return d->artist;
        else
            return QString();
    }

    QString prettyName() const
    {
        if( d )
            return d->artist;
        else
            return QString();
    }

    Track::Private * const d;
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
        if( !d || d->albumArt.isNull() )
            return Meta::Album::image( size, withShadow );
        //TODO implement shadow
        //TODO improve this
        if( d->albumArt.width() != size )
            return d->albumArt.scaled( size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        else
            return d->albumArt;
    }

    Track::Private * const d;
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
};

#endif
