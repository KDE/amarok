/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
Image*                                                                                      *
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

#include "core/meta/Meta.h"
#include "scripting/scriptengine/ScriptingDefines.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"

#include <QScriptEngine>

using namespace AmarokScript;

#define GET_TRACK  Meta::TrackPtr track = qscriptvalue_cast<Meta::TrackPtr>( thisObject() );
#define GET_TRACK_EC( X ) GET_TRACK \
Meta::TrackEditorPtr ec = track->editor(); \
if( ec ) \
{ \
    X; \
}

MetaTrackPrototype::MetaTrackPrototype( QScriptEngine *engine )
    : QObject( engine )
{
    qScriptRegisterMetaType<Meta::TrackList>(engine, toScriptArray, fromScriptArray );
    engine->setDefaultPrototype( qMetaTypeId<Meta::TrackPtr>(),
                                 engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents ) );
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
    return track ? track->statistics()->score() : 0.0;
}

int
MetaTrackPrototype::rating() const
{
    GET_TRACK
    return track ? track->statistics()->rating() : 0;
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
    return track ? track->statistics()->playCount() : 0;
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

int
MetaTrackPrototype::year() const
{
    GET_TRACK
    return ( track && track->year() ) ? track->year()->year() : 0;
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

double
MetaTrackPrototype::bpm() const
{
    GET_TRACK
    return track ? track->bpm() : 0.0;
}

QScriptValue
MetaTrackPrototype::imagePixmap( int size ) const
{
    GET_TRACK
    return ( track && track->album() ) ? thisObject().engine()->toScriptValue( track->album()->image( size ) ) : QScriptValue();
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
    return track->editor(); // converts to bool nicely
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
    if ( track ) track->statistics()->setScore( score );
}

void
MetaTrackPrototype::setRating( int rating )
{
    GET_TRACK
    if ( track ) track->statistics()->setRating( rating );
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
MetaTrackPrototype::setAlbum( const QString &album )
{
    GET_TRACK_EC( ec->setAlbum( album ) )
}

void
MetaTrackPrototype::setArtist( const QString &artist )
{
    GET_TRACK_EC( ec->setArtist( artist ) )
}

void
MetaTrackPrototype::setComposer( const QString &composer )
{
    GET_TRACK_EC( ec->setComposer( composer ) )
}

void
MetaTrackPrototype::setGenre( const QString &genre )
{
    GET_TRACK_EC( ec->setGenre( genre ) )
}

void
MetaTrackPrototype::setYear( int year )
{
    GET_TRACK_EC( ec->setYear( year ) )
}

void
MetaTrackPrototype::setComment( const QString &comment )
{
    GET_TRACK_EC( ec->setComment( comment ) )
}

void
MetaTrackPrototype::setLyrics( const QString &lyrics )
{
    GET_TRACK
    if( track )
        track->setCachedLyrics( lyrics );
}

void
MetaTrackPrototype::setTitle( const QString& title )
{
    GET_TRACK_EC( ec->setTitle( title ) )
}

void
MetaTrackPrototype::setImageUrl( const QString& imageUrl )
{
    GET_TRACK
    if( track && track->album() )
        track->album()->setImage( QImage(imageUrl) );
}

#undef GET_TRACK
#undef GET_TRACK_EC

#include "MetaTypeExporter.moc"
