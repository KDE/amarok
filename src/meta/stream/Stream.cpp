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

#include "Stream.h"
#include "Stream_p.h"

#include "enginecontroller.h"
#include "engineobserver.h"
#include "meta.h"
#include "meta/MetaConstants.h"

#include <QSet>
#include <QString>

using namespace MetaStream;

class Track::Private : public EngineObserver
{
public:
    Private( Track *t )
        : EngineObserver( EngineController::instance() )
        , track( t )
    {}
    void notify() const
    {
        foreach( Meta::Observer *observer, observers )
            observer->metadataChanged( track );
    }

    void newMetaData( QHash<qint64, QString> metaData, bool trackChanged )
    {
        Q_UNUSED( trackChanged )
        if( metaData.contains( Meta::valArtist ) )
            artist = metaData.value( Meta::valArtist );
        if( metaData.contains( Meta::valTitle ) )
            title = metaData.value( Meta::valTitle );
        if( metaData.contains( Meta::valAlbum ) )
            album = metaData.value( Meta::valAlbum );
        notify();
    }

public:
    QSet<Meta::Observer*> observers;
    KUrl url;
    QString title;
    QString artist;
    QString album;

private:
    Track *track;
};

Track::Track( const KUrl &url )
    : Meta::Track()
    , d( new Track::Private( this ) )
{
    d->url = url;
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    if( d->title.isEmpty() )
        return d->url.url();
    else
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
    return d->url.url();
}

QString
Track::url() const
{
    return d->url.url();
}

bool
Track::isPlayable() const
{
    //simple implementation, check internet connectivity or ping server?
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
    return Meta::AlbumPtr();
}

Meta::ArtistPtr
Track::artist() const
{
    //TODO
    return Meta::ArtistPtr();
}

Meta::GenrePtr
Track::genre() const
{
    //TODO
    return Meta::GenrePtr();
}

Meta::ComposerPtr
Track::composer() const
{
    //TODO
    return Meta::ComposerPtr();
}

Meta::YearPtr
Track::year() const
{
    //TODO
    return Meta::YearPtr();
}

void
Track::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
Track::setArtist( const QString& newArtist )
{
    Q_UNUSED( newArtist )
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
    Q_UNUSED( newTitle )
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

int
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

Collection*
Track::collection() const
{
    return 0;
}

void
Track::subscribe( Meta::Observer *observer )
{
    d->observers.insert( observer );
}

void
Track::unsubscribe( Meta::Observer *observer )
{
    d->observers.remove( observer );
}

