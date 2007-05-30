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

#include "debug.h"

#include <QList>

class Track::Private
{
    public:
        KUrl proxyUrl;
        QString lastFmUri;

        QList<TrackObserver*> observers;
};

Track::Track()
    : Meta::Track()
    , QObject()
    , d( new Private() )
{
    //do nothing for now
}

Track::~Track()
{
    delete d;
}

QString
Track::name()
{
    return QString(); //TODO
}

QString
Track::prettyName()
{
    return QString(); //TODO
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
    //TODO create proxy if it doesn't exist yet
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
    d->lastFmUri;
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
    //TODO
}

Meta::ArtistPtr
Track::artist() const
{
    //TODO
}

Meta::GenrePtr
Track::genre() const
{
    //TODO
}

Meta::ComposerPtr
Track::composer() const
{
    //TODO
}

Meta::YearPtr
Track::year() const
{
    //TODO
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
    //TODO
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

int
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
Track::subscribe( TrackObserver *observer )
{
    if( observer && !d->observers.contains( observer ) )
        d->observers.append( observer );
}

void
Track::unsubscribe( TrackObserver *observer )
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

