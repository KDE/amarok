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

#include "LastFmMeta.h"
#include "LastFmMeta_p.h"
#include "LastFmMeta_p.moc"

#include "debug.h"

#include "lastfm.h"

#include <QPointer>

#include <KSharedPtr>

using namespace LastFm;

class LastFmArtist;
class LastFmAlbum;
class LastFmGenre;
class LastFmComposer;
class LastFmYear;

Track::Track( const QString &lastFmUri )
    : QObject()
    , Meta::Track()
    , d( new Private() )
{
    d->lastFmUri = lastFmUri;
    d->t = this;
    d->length = 0;

    d->albumPtr = Meta::AlbumPtr( new LastFmAlbum( QPointer<Track::Private>( d ) ) );
    d->artistPtr = Meta::ArtistPtr( new LastFmArtist( QPointer<Track::Private>( d ) ) );
    d->genrePtr = Meta::GenrePtr( new LastFmGenre( QPointer<Track::Private>( d ) ) );
    d->composerPtr = Meta::ComposerPtr( new LastFmComposer( QPointer<Track::Private>( d ) ) );
    d->yearPtr = Meta::YearPtr( new LastFmYear( QPointer<Track::Private>( d ) ) );
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    //TODO
    if( d->track.isEmpty() )
        return d->lastFmUri;
    else
        return d->track;
}

QString
Track::prettyName() const
{
    //return QString(); //TODO
    QString name = "%1 from %2";
    QStringList tokens = d->lastFmUri.split( '/', QString::SkipEmptyParts );
    if( tokens[1] == "user" )
    {
        name.arg( tokens[3] );
    }
    else if( tokens[1] == "globaltags" )
    {
    }
    else
    {
        //what else??
    }
    return name;
}

QString
Track::fullPrettyName() const
{
    return QString(); //TODO
}

QString
Track::sortableName() const
{
    return QString(); //TODO
}

KUrl
Track::playableUrl() const
{
    if( !d->proxyUrl.isValid() )
    {
        d->proxyUrl = LastFm::Controller::instance()->getNewProxy( d->lastFmUri );
        d->service = LastFm::Controller::instance()->getService();
        if( !d->service )
            return KUrl();
        d->service->addObserver( d );
    }
    return d->proxyUrl;
}

QString
Track::prettyUrl() const
{
    return d->lastFmUri;
}

QString
Track::url() const
{
    return d->lastFmUri;
}

bool
Track::isPlayable() const
{
    //we could check connectivity here...
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
    Q_UNUSED( newAlbum ); //stream
}

void
Track::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist ); //stream
}

void
Track::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre ); //stream
}

void
Track::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer ); //stream
}

void
Track::setYear( const QString &newYear )
{
    Q_UNUSED( newYear ); //stream
}

QString
Track::comment() const
{
    return QString();
}

void
Track::setComment( const QString &newComment )
{
    Q_UNUSED( newComment ); //stream
}

double
Track::score() const
{
    return 0.0;
}

void
Track::setScore( double newScore )
{
    Q_UNUSED( newScore ); //stream
}

int
Track::rating() const
{
    return 0;
}

void
Track::setRating( int newRating )
{
    Q_UNUSED( newRating ); //stream
}

int
Track::trackNumber() const
{
    return 0;
}

void
Track::setTrackNumber( int newTrackNumber )
{
    Q_UNUSED( newTrackNumber ); //stream
}

int
Track::discNumber() const
{
    return 0;
}

void
Track::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber ); //stream
}

int
Track::length() const
{
    return d->length;
}

int
Track::filesize() const
{
    return 0; //stream
}

int
Track::sampleRate() const
{
    return 0; //does the engine deliver this?
}

int
Track::bitrate() const
{
    return 0; //does the engine deliver this??
}

uint
Track::lastPlayed() const
{
    return 0; //TODO do we need this?
}

int
Track::playCount() const
{
    return 0; //TODO do we need this?
}

QString
Track::type() const
{
    return "stream/lastfm";
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
    //TODO
}

bool
Track::inCollection() const
{
    return false;
}

Collection*
Track::collection() const
{
    return 0;
}

void
Track::subscribe( Meta::TrackObserver *observer )
{
    if( observer && !d->observers.contains( observer ) )
        d->observers.append( observer );
}

void
Track::unsubscribe( Meta::TrackObserver *observer )
{
    if( observer )
        d->observers.removeAll( observer );
}

void
Track::love()
{
    //TODO
}

void
Track::ban()
{
    //TODO
}

void
Track::skip()
{
    //TODO
}

#include "LastFmMeta.moc"

