/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_METAPROXY_P_H
#define AMAROK_METAPROXY_P_H

#include "core/collections/Collection.h"
#include "core/meta/Meta.h"
#include "core/meta/Observer.h"
#include "core-impl/meta/stream/Stream.h"

#include <QImage>
#include <QList>
#include <QObject>
#include <QStringList>

#include <kio/job.h>


using namespace MetaProxy;

class MetaProxy::Track::Private : public QObject, public Meta::Observer
{
    Q_OBJECT

    public:
        Track *proxy;
        QUrl url;

        Meta::TrackPtr realTrack;

        QString cachedArtist;
        QString cachedAlbum;
        QString cachedName;
        QString cachedGenre;
        QString cachedComposer;
        int cachedYear;
        qint64 cachedLength;
        qreal  cachedBpm;
        int cachedTrackNumber;
        int cachedDiscNumber;

        Meta::ArtistPtr artistPtr;
        Meta::AlbumPtr albumPtr;
        Meta::GenrePtr genrePtr;
        Meta::ComposerPtr composerPtr;
        Meta::YearPtr yearPtr;

    public:
        using Observer::metadataChanged;
        void metadataChanged( const Meta::TrackPtr &track ) override
        {
            Q_UNUSED( track )
            proxy->notifyObservers();
        }

    public Q_SLOTS:
        void slotUpdateTrack( const Meta::TrackPtr &track )
        {
            if( track )
            {
                // special handling for streams that cannot fetch metadata until played, bug 305389
                MetaStream::Track *stream = dynamic_cast<MetaStream::Track *>( track.data() );
                if( stream )
                    stream->setInitialInfo( cachedArtist, cachedAlbum, cachedName,
                                            cachedLength, cachedTrackNumber );

                subscribeTo( track );
                realTrack = track;

                // clear memory of now-unused cached fields:
                url.clear();
                cachedArtist.clear();
                cachedAlbum.clear();
                cachedName.clear();
                cachedGenre.clear();
                cachedComposer.clear();

                proxy->notifyObservers();
            }
        }
};

// internal helper classes

class ProxyArtist : public Meta::Artist
{
public:
    explicit ProxyArtist( MetaProxy::Track::Private *dptr )
        : Meta::Artist()
        , d( dptr )
    {}

    Meta::TrackList tracks() override
    {
        Meta::TrackPtr realTrack = d ? d->realTrack : Meta::TrackPtr();
        Meta::ArtistPtr artist = realTrack ? realTrack->artist() : Meta::ArtistPtr();
        return artist ? artist->tracks() : Meta::TrackList();
    }

    QString name() const override
    {
        Meta::TrackPtr realTrack = d ? d->realTrack : Meta::TrackPtr();
        if( realTrack )
        {
            Meta::ArtistPtr artist = realTrack ? realTrack->artist() : Meta::ArtistPtr();
            return artist ? artist->name() : QString();
        }
        return d ? d->cachedArtist : QString();
    }

    QString prettyName() const override
    {
        Meta::TrackPtr realTrack = d ? d->realTrack : Meta::TrackPtr();
        if( realTrack )
        {
            Meta::ArtistPtr artist = realTrack ? realTrack->artist() : Meta::ArtistPtr();
            return artist ? artist->prettyName() : QString();
        }
        return d ? d->cachedArtist : QString();
    }

    bool operator==( const Meta::Artist &artist ) const override
    {
        const ProxyArtist *proxy = dynamic_cast<const ProxyArtist*>( &artist );
        if( proxy )
        {
            return d && proxy->d && d->realTrack && proxy->d->realTrack && d->realTrack->artist() && d->realTrack->artist() == proxy->d->realTrack->artist();
        }
        else
        {
            return d && d->realTrack && d->realTrack->artist() && d->realTrack->artist().data() == &artist;
        }
    }

    MetaProxy::Track::Private * const d;
};

/** TODO: what about MetaDataChanged? */
class ProxyAlbum : public Meta::Album
{
public:
    explicit ProxyAlbum( MetaProxy::Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
    {}

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->hasCapabilityInterface( type );
        else
            return false;
    }

    Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->createCapabilityInterface( type );
        else
            return nullptr;
    }

    bool isCompilation() const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->isCompilation();
        else
            return false;
    }

    bool canUpdateCompilation() const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->canUpdateCompilation();
        else
            return Meta::Album::canUpdateCompilation();
    }

    void setCompilation( bool isCompilation ) override
    {
        if( d && d->realTrack && d->realTrack->album() )
            d->realTrack->album()->setCompilation( isCompilation );
    }

    bool hasAlbumArtist() const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->hasAlbumArtist();
        else
            return false;
    }

    Meta::ArtistPtr albumArtist() const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->albumArtist();
        else
            return Meta::ArtistPtr();
    }

    Meta::TrackList tracks() override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->tracks();
        else
            return Meta::TrackList();
    }

    QString name() const override
    {
        if( d && d->realTrack )
        {
            if ( d->realTrack->album() )
                return d->realTrack->album()->name();
            return QString();
        }
        else if ( d )
              return d->cachedAlbum;
        else
            return QString();
    }

    QString prettyName() const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->prettyName();
        else
            return name();
    }

    QImage image( int size ) const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->image( size );
        else
            return Meta::Album::image( size );
    }

    bool hasImage( int size ) const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->hasImage( size );
        else
            return Meta::Album::hasImage( size );
    }

    QUrl imageLocation( int size = 0 ) override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->imageLocation( size );
        else
            return Meta::Album::imageLocation( size );
    }

    bool canUpdateImage() const override
    {
        if( d && d->realTrack && d->realTrack->album() )
            return d->realTrack->album()->canUpdateImage();
        else
            return Meta::Album::canUpdateImage();
    }

    void setImage( const QImage &image ) override
    {
        if( d && d->realTrack && d->realTrack->album() )
            d->realTrack->album()->setImage( image );
    }

    void removeImage() override
    {
        if( d && d->realTrack && d->realTrack->album() )
            d->realTrack->album()->removeImage();
    }

    bool operator==( const Meta::Album &album ) const override
    {
        const ProxyAlbum *proxy = dynamic_cast<const ProxyAlbum*>( &album );
        if( proxy )
        {
            return d && proxy->d && d->realTrack && proxy->d->realTrack && d->realTrack->album() && ( *d->realTrack->album().data() ) == ( *proxy->d->realTrack->album().data() );
        }
        else
        {
            return d && d->realTrack && d->realTrack->album() && ( *d->realTrack->album().data() ) == album;
        }
    }

    MetaProxy::Track::Private * const d;
};

class ProxyGenre : public Meta::Genre
{
public:
    explicit ProxyGenre( MetaProxy::Track::Private *dptr )
        : Meta::Genre()
        , d( dptr )
    {}

    QString name() const override
    {
        if( d && d->realTrack && d->realTrack->genre() )
            return d->realTrack->genre()->name();
        else if( d )
            return d->cachedGenre;
        else
            return QString();
    }

    QString prettyName() const override
    {
        if( d && d->realTrack && d->realTrack->genre() )
            return d->realTrack->genre()->prettyName();
        else
            return QString();
    }

    Meta::TrackList tracks() override
    {
        if( d && d->realTrack && d->realTrack->genre() )
            return d->realTrack->genre()->tracks();
        else
            return Meta::TrackList();
    }

    bool operator==( const Meta::Genre &genre ) const override
    {
        const ProxyGenre *proxy = dynamic_cast<const ProxyGenre*>( &genre );
        if( proxy )
        {
            return d && proxy->d && d->realTrack && proxy->d->realTrack && d->realTrack->genre() && d->realTrack->genre() == proxy->d->realTrack->genre();
        }
        else
        {
            return d && d->realTrack && d->realTrack->genre() && d->realTrack->genre().data() == &genre;
        }
    }

    MetaProxy::Track::Private * const d;
};

class ProxyComposer : public Meta::Composer
{
public:
    explicit ProxyComposer( MetaProxy::Track::Private *dptr )
        : Meta::Composer()
        , d( dptr )
    {}

    QString name() const override
    {
        if( d && d->realTrack && d->realTrack->composer() )
            return d->realTrack->composer()->name();
        else if ( d )
            return d->cachedComposer;
        else
            return QString();
    }

    QString prettyName() const override
    {
        if( d && d->realTrack && d->realTrack->composer())
            return d->realTrack->composer()->prettyName();
        else
            return name();
    }

    Meta::TrackList tracks() override
    {
        if( d && d->realTrack && d->realTrack->composer() )
            return d->realTrack->composer()->tracks();
        else
            return Meta::TrackList();
    }

    bool operator==( const Meta::Composer &composer ) const override
    {
        const ProxyComposer *proxy = dynamic_cast<const ProxyComposer*>( &composer );
        if( proxy )
        {
            return d && proxy->d && d->realTrack && proxy->d->realTrack && d->realTrack->composer() && d->realTrack->composer() == proxy->d->realTrack->composer();
        }
        else
        {
            return d && d->realTrack && d->realTrack->composer() && d->realTrack->composer().data() == &composer;
        }
    }

    MetaProxy::Track::Private * const d;
};

class ProxyYear : public Meta::Year
{
public:
    explicit ProxyYear( MetaProxy::Track::Private *dptr )
        : Meta::Year()
        , d( dptr )
    {}

    QString name() const override
    {
        if( d && d->realTrack && d->realTrack->year() )
            return d->realTrack->year()->name();
        else if( d )
            return QString::number(d->cachedYear);
        else
            return QString();
    }

    QString prettyName() const override
    {
        if( d && d->realTrack && d->realTrack->year() )
            return d->realTrack->year()->prettyName();
        else
            return name();
    }

    Meta::TrackList tracks() override
    {
        if( d && d->realTrack && d->realTrack->year() )
            return d->realTrack->year()->tracks();
        else
            return Meta::TrackList();
    }

    bool operator==( const Meta::Year &year ) const override
    {
        const ProxyYear *proxy = dynamic_cast<const ProxyYear*>( &year );
        if( proxy )
        {
            return d && proxy->d && d->realTrack && proxy->d->realTrack && d->realTrack->year() && d->realTrack->year() == proxy->d->realTrack->year();
        }
        else
        {
            return d && d->realTrack && d->realTrack->year() && d->realTrack->year().data() == &year;
        }
    }

    MetaProxy::Track::Private * const d;
};

#endif
