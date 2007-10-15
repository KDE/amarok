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

#ifndef AMAROK_METAPROXY_P_H
#define AMAROK_METAPROXY_P_H

#include "debug.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "lastfm.h"
#include "meta.h"

#include <QImage>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QStringList>

#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace MetaProxy;

class MetaProxy::Track::Private : public QObject, public Meta::Observer
{
    Q_OBJECT

    public:
        Track *proxy;
        KUrl url;

        Meta::TrackPtr realTrack;

        QList<Meta::Observer*> observers;

        QString cachedArtist;
        QString cachedAlbum;
        QString cachedName;
        int cachedLength;

        Meta::ArtistPtr artistPtr;
        Meta::AlbumPtr albumPtr;
        Meta::GenrePtr genrePtr;
        Meta::ComposerPtr composerPtr;
        Meta::YearPtr yearPtr;

    public:
        void notifyObservers()
        {
            foreach( Meta::Observer *observer, observers )
                observer->metadataChanged( proxy );
        }

        void metadataChanged( Meta::Track *track )
        {
            Q_UNUSED( track )
            notifyObservers();
        }

    public slots:
        void slotNewCollection( Collection *newCollection )
        {
            if( newCollection->possiblyContainsTrack( url ) )
            {
                Meta::TrackPtr track = newCollection->trackForUrl( url );
                if( track )
                {
                    track->subscribe( this );
                    realTrack = track;
                    notifyObservers();
                    disconnect( CollectionManager::instance(), SIGNAL( collectionAdded( Collection* ) ), this, SLOT( slotNewCollection( Collection* ) ) );
                }
            }
        }

        void slotCheckCollectionManager()
        {
            Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
            if( track )
                realTrack = track;
            notifyObservers();
            disconnect( CollectionManager::instance(), SIGNAL( collectionAdded( Collection* ) ), this, SLOT( slotNewCollection( Collection* ) ) );
        }
};

// internal helper classes

class ProxyArtist : public Meta::Artist
{
public:
    ProxyArtist( MetaProxy::Track::Private *dptr )
        : Meta::Artist()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        if( d && d->realTrack )
            return d->realTrack->artist()->tracks();
        else
            return Meta::TrackList();
    }

    QString name() const
    {
        if( d && d->realTrack ) {
            if ( d->realTrack->artist() )
                return d->realTrack->artist()->name();
            return QString();
        } else if( d )
            return d->cachedArtist;
        return QString();
    }

    QString prettyName() const
    {
        if( d && d->realTrack )
            return d->realTrack->artist()->prettyName();
        else if( d )
            return d->cachedArtist;
        return QString();
    }

    MetaProxy::Track::Private * const d;
};

class ProxyAlbum : public Meta::Album
{
public:
    ProxyAlbum( MetaProxy::Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
    {}

    bool isCompilation() const
    {
        if( d && d->realTrack )
            return d->realTrack->album()->isCompilation();
        else
            return false;
    }

    bool hasAlbumArtist() const
    {
        if( d && d->realTrack )
            return d->realTrack->album()->hasAlbumArtist();
        else
            return false;
    }

    Meta::ArtistPtr albumArtist() const
    {
        if( d && d->realTrack )
            return d->realTrack->album()->albumArtist();
        else
            return Meta::ArtistPtr();
    }

    Meta::TrackList tracks()
    {
        if( d && d->realTrack )
            return d->realTrack->album()->tracks();
        else
            return Meta::TrackList();
    }

    QString name() const
    {
        if( d && d->realTrack ) {
            if ( d->realTrack->album() )
                return d->realTrack->album()->name();
            return QString();
        } else
            return QString();
    }

    QString prettyName() const
    {
        if( d && d->realTrack )
            return d->realTrack->album()->prettyName();
        else
            return QString();
    }

    QPixmap image( int size, bool withShadow ) const
    {
        if( d && d->realTrack ) {
            if ( d->realTrack->album() )
                return d->realTrack->album()->image( size, withShadow );
            return Meta::Album::image( size, withShadow );
        } else
            return Meta::Album::image( size, withShadow );
    }

    MetaProxy::Track::Private * const d;
};

class ProxyGenre : public Meta::Genre
{
public:
    ProxyGenre( MetaProxy::Track::Private *dptr )
        : Meta::Genre()
        , d( dptr )
    {}

    QString name() const
    {
        if( d && d->realTrack )
            return d->realTrack->genre()->name();
        else
            return QString();
    }

    QString prettyName() const
    {
        if( d && d->realTrack )
            return d->realTrack->genre()->prettyName();
        else
            return QString();
    }

    Meta::TrackList tracks()
    {
        if( d && d->realTrack )
            return d->realTrack->genre()->tracks();
        else
            return Meta::TrackList();
    }

    MetaProxy::Track::Private * const d;
};

class ProxyComposer : public Meta::Composer
{
public:
    ProxyComposer( MetaProxy::Track::Private *dptr )
        : Meta::Composer()
        , d( dptr )
    {}

    QString name() const
    {
        if( d && d->realTrack )
            return d->realTrack->composer()->name();
        else
            return QString();
    }

    QString prettyName() const
    {
        if( d && d->realTrack )
            return d->realTrack->composer()->prettyName();
        else
            return QString();
    }

    Meta::TrackList tracks()
    {
        if( d && d->realTrack )
            return d->realTrack->composer()->tracks();
        else
            return Meta::TrackList();
    }

    MetaProxy::Track::Private * const d;
};

class ProxyYear : public Meta::Year
{
public:
    ProxyYear( MetaProxy::Track::Private *dptr )
        : Meta::Year()
        , d( dptr )
    {}

    QString name() const
    {
        if( d && d->realTrack )
            return d->realTrack->year()->name();
        else
            return QString();
    }

    QString prettyName() const
    {
        if( d && d->realTrack )
            return d->realTrack->year()->prettyName();
        else
            return QString();
    }

    Meta::TrackList tracks()
    {
        if( d && d->realTrack )
            return d->realTrack->year()->tracks();
        else
            return Meta::TrackList();
    }

    MetaProxy::Track::Private * const d;
};

#endif
