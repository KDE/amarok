/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "core-implementations/meta/stream/Stream.h"
#include "core-implementations/meta/stream/Stream_p.h"

#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core/meta/impl/default/DefaultMetaTypes.h"

#include <QPointer>
#include <QString>

using namespace MetaStream;

Track::Track( const KUrl &url )
    : Meta::Track()
    , d( new Track::Private( this ) )
{
    DEBUG_BLOCK

    d->url = url;
    d->artistPtr = Meta::ArtistPtr( new StreamArtist( QPointer<Track::Private>( d ) ) );
    d->albumPtr = Meta::AlbumPtr( new StreamAlbum( QPointer<Track::Private>( d ) ) );
    d->genrePtr = Meta::GenrePtr( new Meta::DefaultGenre() );
    d->composerPtr = Meta::ComposerPtr( new Meta::DefaultComposer() );
    d->yearPtr = Meta::YearPtr( new Meta::DefaultYear() );
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    if( d->title.isEmpty() )
        return i18n( "Stream (%1)", d->url.url() );
    return d->title;
}

QString
Track::prettyName() const
{
    return name();
}

QString
Track::fullPrettyName() const
{
    return name();
}

QString
Track::sortableName() const
{
    return name();
}


KUrl
Track::playableUrl() const
{
    return d->url;
}

QString
Track::prettyUrl() const
{
    return playableUrl().url();
}

QString
Track::uidUrl() const
{
    return playableUrl().url();
}

bool
Track::isPlayable() const
{
    //simple implementation, check Internet connectivity or ping server?
    return true;
}

bool
Track::isEditable() const
{
    return false;
}

Meta::AlbumPtr
Track::album() const
{
    return d->albumPtr;
}

Meta::ArtistPtr
Track::artist() const
{
    return d->artistPtr;
}

Meta::GenrePtr
Track::genre() const
{
    return d->genrePtr;
}

Meta::ComposerPtr
Track::composer() const
{
    return d->composerPtr;
}

Meta::YearPtr
Track::year() const
{
    return d->yearPtr;
}

void
Track::setAlbum( const QString &newAlbum )
{
    d->album = newAlbum;
}

void
Track::setArtist( const QString& newArtist )
{
    d->artist = newArtist;
}

void
Track::setGenre( const QString& newGenre )
{
    Q_UNUSED( newGenre )
}

void
Track::setComposer( const QString& newComposer )
{
    Q_UNUSED( newComposer )
}

void
Track::setYear( const QString& newYear )
{
    Q_UNUSED( newYear )
}

void
Track::setTitle( const QString &newTitle )
{
    //it is sometimes useful to set a title for a stream so it has a nice name
    //before we actually start playing it
    d->title = newTitle;
}

qreal
Track::bpm() const
{
    return -1.0;
}

QString
Track::comment() const
{
    return QString();
}

void
Track::setComment( const QString& newComment )
{
    Q_UNUSED( newComment )
}

double
Track::score() const
{
    return 0.0;
}

void
Track::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
Track::rating() const
{
    return 0;
}

void
Track::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
Track::trackNumber() const
{
    return 0;
}

void
Track::setTrackNumber( int newTrackNumber )
{
    Q_UNUSED( newTrackNumber )
}

int
Track::discNumber() const
{
    return 0;
}

void
Track::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

qint64
Track::length() const
{
    //TODO
    return 0;
}

int
Track::filesize() const
{
    return 0;
}

int
Track::sampleRate() const
{
    return 0;
}

int
Track::bitrate() const
{
    return 0;
}

uint
Track::lastPlayed() const
{
    return 0;
}

int
Track::playCount() const
{
    return 0;
}

QString
Track::type() const
{
    return "stream";
}

void
Track::beginMetaDataUpdate()
{
    //not editable
}

void
Track::endMetaDataUpdate()
{
    //not editable
}

void
Track::abortMetaDataUpdate()
{
    //not editable
}

void
Track::finishedPlaying( double playedFraction )
{
    Q_UNUSED( playedFraction );
    //TODO
}

bool
Track::inCollection() const
{
    return false;
}

Amarok::Collection*
Track::collection() const
{
    return 0;
}

void
Track::subscribe( Meta::Observer *observer )
{
    DEBUG_BLOCK

    debug() << "Adding observer: " << observer;
    d->observers.insert( observer );
}

void
Track::unsubscribe( Meta::Observer *observer )
{
    DEBUG_BLOCK

    debug() << "Removing observer: " << observer;
    d->observers.remove( observer );
}

void Track::updateUrl( const KUrl & url )
{
    d->url = url;
    notifyObservers();
}

#include "Stream_p.moc"

