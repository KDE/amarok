/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
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

#include "MetaTypeExporter.h"

#include <QtScript>

#define GET_TRACK  Meta::TrackPtr track = qscriptvalue_cast<Meta::TrackPtr>( thisObject() );
#define GET_TRACK_EC( X ) Meta::TrackPtr track = qscriptvalue_cast<Meta::TrackPtr>( thisObject() ); \
Meta::EditCapability* ec = track->create<Meta::EditCapability>(); \
if( ec ) \
{ \
    ec->beginMetaDataUpdate(); \
    X; \
    ec->endMetaDataUpdate(); \
}

MetaTrackPrototype::MetaTrackPrototype()
{
}

MetaTrackPrototype::~MetaTrackPrototype()
{
}

int
MetaTrackPrototype::sampleRate() const
{
    GET_TRACK
    return track ? track->sampleRate() : 0;
}

int
MetaTrackPrototype::bitrate() const
{
    GET_TRACK
    return track ? track->bitrate() : 0;
}

double
MetaTrackPrototype::score() const
{
    GET_TRACK
    return track ? track->score() : 0.0;
}

int
MetaTrackPrototype::rating() const
{
    GET_TRACK
    return track ? track->rating() : 0;
}

bool
MetaTrackPrototype::inCollection() const
{
    GET_TRACK
    return track ? track->inCollection() : false;
}

QString
MetaTrackPrototype::type() const
{
    GET_TRACK
    return track ? track->type() : QString();
}

qint64
MetaTrackPrototype::length() const
{
    GET_TRACK
    return track ? track->length() : 0;
}

int
MetaTrackPrototype::fileSize() const
{
    GET_TRACK
    return track ? track->filesize() : 0;
}

int
MetaTrackPrototype::trackNumber() const
{
    GET_TRACK
    return track ? track->trackNumber() : 0;
}

int
MetaTrackPrototype::discNumber() const
{
    GET_TRACK
    return track ? track->discNumber() : 0;
}

int
MetaTrackPrototype::playCount() const
{
    GET_TRACK
    return track ? track->playCount() : 0;
}

bool
MetaTrackPrototype::playable() const
{
    GET_TRACK
    return track ? track->isPlayable() : false;
}

QString
MetaTrackPrototype::album() const
{
    GET_TRACK
    return ( track && track->album() ) ? track->album()->prettyName() : QString();
}

QString
MetaTrackPrototype::artist() const
{
    GET_TRACK
    return ( track && track->artist() ) ? track->artist()->prettyName() : QString();
}

QString
MetaTrackPrototype::composer() const
{
    GET_TRACK
    return ( track && track->composer() ) ? track->composer()->prettyName() : QString();
}

QString
MetaTrackPrototype::genre() const
{
    GET_TRACK
    return ( track && track->genre() ) ? track->genre()->prettyName() : QString();
}

QString
MetaTrackPrototype::year() const
{
    GET_TRACK
    return ( track && track->year() ) ? track->year()->prettyName() : QString();
}

QString
MetaTrackPrototype::comment() const
{
    GET_TRACK
    return track ? track->comment() : QString();
}

QString
MetaTrackPrototype::path() const
{
    GET_TRACK
    return track ? track->playableUrl().path() : QString();
}

QString
MetaTrackPrototype::title() const
{
    GET_TRACK
    return track ? track->prettyName() : QString();
}

QString
MetaTrackPrototype::imageUrl() const
{
    GET_TRACK
    return ( track && track->album() ) ? track->album()->imageLocation().prettyUrl() : QString();
}

QString
MetaTrackPrototype::url() const
{
    GET_TRACK
    return track ? track->playableUrl().url() : QString();
}

QScriptValue
MetaTrackPrototype::imagePixmap( int size ) const
{
    GET_TRACK
    return ( track && track->album() ) ? thisObject().engine()->toScriptValue( track->album()->image( size ) ) : QScriptValue();
}

QScriptValue
MetaTrackPrototype::imagePixmap() const
{
    return imagePixmap( 1 );
}

bool
MetaTrackPrototype::isValid() const
{
    GET_TRACK
    if ( track ) return true;
    return false;
}
bool
MetaTrackPrototype::isEditable() const
{
    GET_TRACK
    Meta::EditCapability* ec = track->create<Meta::EditCapability>();
    return ( ec && ec->isEditable() );
}

QString
MetaTrackPrototype::lyrics() const
{
    GET_TRACK
    return track ? track->cachedLyrics() : QString();
}

void
MetaTrackPrototype::setScore( double score )
{
    GET_TRACK
    if ( track ) track->setScore( score );
}

void
MetaTrackPrototype::setRating( int rating )
{
    GET_TRACK
    if ( track ) track->setRating( rating );
}

void
MetaTrackPrototype::setTrackNumber( int number )
{
    GET_TRACK_EC( ec->setTrackNumber( number ) )
}

void
MetaTrackPrototype::setDiscNumber( int number )
{
    GET_TRACK_EC( ec->setDiscNumber( number ) )
}

void
MetaTrackPrototype::setAlbum( QString album )
{
    GET_TRACK_EC( ec->setAlbum( album ) )
}

void
MetaTrackPrototype::setArtist( QString artist )
{
    GET_TRACK_EC( ec->setArtist( artist ) )
}

void
MetaTrackPrototype::setComposer( QString composer )
{
    GET_TRACK_EC( ec->setComposer( composer ) )
}

void
MetaTrackPrototype::setGenre( QString genre )
{
    GET_TRACK_EC( ec->setGenre( genre ) )
}

void
MetaTrackPrototype::setYear( QString year )
{
    GET_TRACK_EC( ec->setYear( year ) )
}

void
MetaTrackPrototype::setComment( QString comment )
{
    GET_TRACK_EC( ec->setComment( comment ) )
}

void
MetaTrackPrototype::setLyrics( QString lyrics )
{
    GET_TRACK
    if ( track ) track->setCachedLyrics( lyrics );
}

void
MetaTrackPrototype::setTitle( const QString& title )
{
    GET_TRACK_EC( ec->setTitle( title ) )
}

void
MetaTrackPrototype::setImageUrl(const QString& imageUrl )
{
    GET_TRACK
    if ( track && track->album() ) track->album()->setImage( QPixmap(imageUrl) );
}

#undef GET_TRACK
#undef GET_TRACK_EC

#include "MetaTypeExporter.moc"
