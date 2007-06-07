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

#include "MetaProxy.h"
#include "MetaProxy_p.h"
#include "MetaProxy_p.moc"

#include "debug.h"

#include "collectionmanager.h"

#include <QObject>
#include <QPointer>

#include <KSharedPtr>

using namespace MetaProxy;

class ProxyArtist;
class ProxyFmAlbum;
class ProxyGenre;
class ProxyComposer;
class ProxyYear;

MetaProxy::Track::Track( const KUrl &url )
    : Meta::Track()
    , d( new Private() )
{
    d->url = url;
    d->proxy = this;
    d->cachedLength = 0;

    QObject::connect( CollectionManager::instance(), SIGNAL( collectionAdded( Collection* ) ), d, SLOT( slotNewCollection( Collection* ) ) );

    d->albumPtr = Meta::AlbumPtr( new ProxyAlbum( QPointer<Track::Private>( d ) ) );
    d->artistPtr = Meta::ArtistPtr( new ProxyArtist( QPointer<Track::Private>( d ) ) );
    d->genrePtr = Meta::GenrePtr( new ProxyGenre( QPointer<Track::Private>( d ) ) );
    d->composerPtr = Meta::ComposerPtr( new ProxyComposer( QPointer<Track::Private>( d ) ) );
    d->yearPtr = Meta::YearPtr( new ProxyYear( QPointer<Track::Private>( d ) ) );
}

MetaProxy::Track::~Track()
{
    delete d;
}

QString
MetaProxy::Track::name() const
{
    if( d->realTrack )
        return d->realTrack->name();
    else
        return d->cachedName;
}

QString
MetaProxy::Track::prettyName() const
{
    if( d->realTrack )
        return d->realTrack->prettyName();
    else
        return d->cachedName;   //TODO maybe change this?
}

QString
MetaProxy::Track::fullPrettyName() const
{
    if( d->realTrack )
        return d->realTrack->fullPrettyName();
    else
        return d->cachedName;   //TODO maybe change this??
}

QString
MetaProxy::Track::sortableName() const
{
    if( d->realTrack )
        return d->realTrack->sortableName();
    else
        return d->cachedName;   //TODO change this?
}

KUrl
MetaProxy::Track::playableUrl() const
{
    if( d->realTrack )
        return d->realTrack->playableUrl();
    else
        return KUrl();
}

QString
MetaProxy::Track::prettyUrl() const
{
    if( d->realTrack )
        return d->realTrack->prettyUrl();
    else
        return d->url.url();
}

QString
MetaProxy::Track::url() const
{
    if( d->realTrack )
        return d->realTrack->url();
    else
        return d->url.url();
}

bool
MetaProxy::Track::isPlayable() const
{
    if( d->realTrack )
        return d->realTrack->isPlayable();
    else
        return false;
}

bool
MetaProxy::Track::isEditable() const
{
    if( d->realTrack )
        return d->realTrack->isEditable();
    else
        return false;
}

Meta::AlbumPtr
MetaProxy::Track::album() const
{
    return d->albumPtr;
}

Meta::ArtistPtr
MetaProxy::Track::artist() const
{
    return d->artistPtr;
}

Meta::GenrePtr
MetaProxy::Track::genre() const
{
    return d->genrePtr;
}

Meta::ComposerPtr
MetaProxy::Track::composer() const
{
    return d->composerPtr;
}

Meta::YearPtr
MetaProxy::Track::year() const
{
    return d->yearPtr;
}

void
MetaProxy::Track::setAlbum( const QString &newAlbum )
{
    if( d->realTrack )
        d->realTrack->setAlbum( newAlbum );
}

void
MetaProxy::Track::setArtist( const QString &newArtist )
{
    if( d->realTrack )
        d->realTrack->setArtist( newArtist );
}

void
MetaProxy::Track::setGenre( const QString &newGenre )
{
    if( d->realTrack )
        d->realTrack->setGenre( newGenre );
}

void
MetaProxy::Track::setComposer( const QString &newComposer )
{
    if( d->realTrack )
        d->realTrack->setComposer( newComposer );
}

void
MetaProxy::Track::setYear( const QString &newYear )
{
    if( d->realTrack )
        d->realTrack->setYear( newYear );
}

QString
MetaProxy::Track::comment() const
{
    if( d->realTrack )
        return d->realTrack->comment();
    else
        return QString();       //do we cache the comment??
}

void
MetaProxy::Track::setComment( const QString &newComment )
{
    if( d->realTrack )
        d->realTrack->setComment( newComment );
}

double
MetaProxy::Track::score() const
{
    if( d->realTrack )
        return d->realTrack->score();
    else
        return 0.0;     //do we cache the score
}

void
MetaProxy::Track::setScore( double newScore )
{
    if( d->realTrack )
        d->realTrack->setScore( newScore );
}

int
MetaProxy::Track::rating() const
{
    if( d->realTrack )
        return d->realTrack->rating();
    else
        return 0;
}

void
MetaProxy::Track::setRating( int newRating )
{
    if( d->realTrack )
        d->realTrack->setRating( newRating );
}

int
MetaProxy::Track::trackNumber() const
{
    if( d->realTrack )
        return d->realTrack->trackNumber();
    else
        return 0;
}

void
MetaProxy::Track::setTrackNumber( int newTrackNumber )
{
    if( d->realTrack )
        d->realTrack->setTrackNumber( newTrackNumber );
}

int
MetaProxy::Track::discNumber() const
{
    if( d->realTrack )
        return d->realTrack->discNumber();
    else
        return 0;
}

void
MetaProxy::Track::setDiscNumber( int newDiscNumber )
{
    if( d->realTrack )
        d->realTrack->setDiscNumber( newDiscNumber );
}

int
MetaProxy::Track::length() const
{
    if( d->realTrack )
        return d->realTrack->length();
    else
        return d->cachedLength;
}

int
MetaProxy::Track::filesize() const
{
    if( d->realTrack )
        return d->realTrack->filesize();
    else
        return 0;
}

int
MetaProxy::Track::sampleRate() const
{
    if( d->realTrack )
        return d->realTrack->sampleRate();
    else
        return 0;
}

int
MetaProxy::Track::bitrate() const
{
    if( d->realTrack )
        return d->realTrack->bitrate();
    else
        return 0;
}

uint
MetaProxy::Track::lastPlayed() const
{
    if( d->realTrack )
        return d->realTrack->lastPlayed();
    else
        return 0;
}

int
MetaProxy::Track::playCount() const
{
    if( d->realTrack )
        return d->realTrack->playCount();
    else
        return 0;
}

QString
MetaProxy::Track::type() const
{
    if( d->realTrack )
        return d->realTrack->type();
    else
        return QString();       //TODO cache type??
}

void
MetaProxy::Track::beginMetaDataUpdate()
{
    if( d->realTrack )
        d->realTrack->beginMetaDataUpdate();
}

void
MetaProxy::Track::endMetaDataUpdate()
{
    if( d->realTrack )
        d->realTrack->endMetaDataUpdate();
}

void
MetaProxy::Track::abortMetaDataUpdate()
{
    if( d->realTrack )
        d->realTrack->abortMetaDataUpdate();
}

void
MetaProxy::Track::finishedPlaying( double playedFraction )
{
    if( d->realTrack )
        d->realTrack->finishedPlaying( playedFraction );
}

bool
MetaProxy::Track::inCollection() const
{
    if( d->realTrack )
        return d->realTrack->inCollection();
    else
        return false;
}

Collection*
MetaProxy::Track::collection() const
{
    if( d->realTrack )
        return d->realTrack->collection();
    else
        return 0;
}

void
MetaProxy::Track::subscribe( Meta::TrackObserver *observer )
{
    if( observer && !d->observers.contains( observer ) )
        d->observers.append( observer );
}

void
MetaProxy::Track::unsubscribe( Meta::TrackObserver *observer )
{
    if( observer )
        d->observers.removeAll( observer );
}


