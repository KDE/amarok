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
#include "MetaUtility.h"

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
    d->genre = Meta::GenrePtr( new MetaFile::FileGenre( QPointer<MetaFile::Track::Private>( d ) ) );
    d->composer = Meta::ComposerPtr( new MetaFile::FileComposer( QPointer<MetaFile::Track::Private>( d ) ) );
    d->year = Meta::YearPtr( new MetaFile::FileYear( QPointer<MetaFile::Track::Private>( d ) ) );
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::TITLE ) );
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
    return d->genre;
}

Meta::ComposerPtr
Track::composer() const
{
    return d->composer;
}

Meta::YearPtr
Track::year() const
{
    return d->year;
}

void
Track::setAlbum( const QString &newAlbum )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::ALBUM ) ).setValue( newAlbum );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

void
Track::setArtist( const QString& newArtist )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::ARTIST ) ).setValue( newArtist );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

void
Track::setGenre( const QString& newGenre )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::GENRE ) ).setValue( newGenre );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

void
Track::setComposer( const QString& newComposer )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::COMPOSER ) ).setValue( newComposer );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

void
Track::setYear( const QString& newYear )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::YEAR ) ).setValue( newYear );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

void
Track::setTitle( const QString &newTitle )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::TITLE ) ).setValue( newTitle );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

QString
Track::comment() const
{
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::COMMENT ) );
    if( item.isValid() )
        return item.value().toString();
    else
        return QString();
}

void
Track::setComment( const QString& newComment )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::COMMENT ) ).setValue( newComment );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
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
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::TRACKNUMBER ) );
    if( item.isValid() )
        return item.value().toInt();
    else
        return 0;
}

void
Track::setTrackNumber( int newTrackNumber )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::TRACKNUMBER ) ).setValue( newTrackNumber );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

int
Track::discNumber() const
{
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::DISCNUMBER ) );
    if( item.isValid() )
        return item.value().toInt();
    else
        return 0;
}

void
Track::setDiscNumber( int newDiscNumber )
{
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::DISCNUMBER ) ).setValue( newDiscNumber );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
}

int
Track::length() const
{
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::LENGTH ) );
    if( item.isValid() )
        return item.value().toInt();
    else
        return 0;
}

int
Track::filesize() const
{
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::FILESIZE ) );
    if( item.isValid() )
        return item.value().toInt();
    else
        return 0;
}

int
Track::sampleRate() const
{
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::SAMPLERATE ) );
    if( item.isValid() )
        return item.value().toInt();
    else
        return 0;
}

int
Track::bitrate() const
{
    KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::BITRATE ) );
    if( item.isValid() )
        return item.value().toInt();
    else
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
    d->batchUpdate = true;
}

void
Track::endMetaDataUpdate()
{
    d->metaInfo.applyChanges();
    d->batchUpdate = false;
    notifyObservers();


}

void
Track::abortMetaDataUpdate()
{
    //KFileMetaInfo does not have a method to reset the items
    d->metaInfo = KFileMetaInfo( d->url.path() );
    d->batchUpdate = false;
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

#include "File.moc"
