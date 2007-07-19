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

#include "File.h"
#include "File_p.h"

#include "debug.h"
#include "meta.h"

#include <QFile>
#include <QPointer>
#include <QString>

#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>

using namespace MetaFile;

Track::Track( const KUrl &url )
    : Meta::Track()
    , d( new Track::Private( this ) )
{
    d->url = url;
    d->metaInfo = KFileMetaInfo( url.path() );
    d->album = Meta::AlbumPtr( new MetaFile::FileAlbum( QPointer<MetaFile::Track::Private>( d ) ) );
    d->artist = Meta::ArtistPtr( new MetaFile::FileArtist( QPointer<MetaFile::Track::Private>( d ) ) );
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    KFileMetaInfoItem item = d->metaInfo.item( TITLE );
    if( item.isValid() )
        return item.value().toString();
    else
        return QString();
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
    return d->url.path();
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
    //not this probably needs more work on *nix
    return QFile::permissions( d->url.path() ) & QFile::WriteUser;
}

Meta::AlbumPtr
Track::album() const
{
    return d->album;
}

Meta::ArtistPtr
Track::artist() const
{
    return d->artist;
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
    d->metaInfo.item( ALBUM ).setValue( newAlbum );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        d->notify();
    }
}

void
Track::setArtist( const QString& newArtist )
{
    d->metaInfo.item( ARTIST ).setValue( newArtist );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        d->notify();
    }
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
    d->metaInfo.item( TITLE ).setValue( newTitle );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        d->notify();
    }
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
    KFileMetaInfoItem item = d->metaInfo.item( FILESIZE );
    if( item.isValid() )
        return item.value().toInt();
    else
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
    return "";
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
Track::subscribe( Meta::TrackObserver *observer )
{
    d->observers.insert( observer );
}

void
Track::unsubscribe( Meta::TrackObserver *observer )
{
    d->observers.remove( observer );
}

#include "File.moc"
