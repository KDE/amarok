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

#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/jobs/WriteTagsJob.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "MetaTagLib.h"
#include "scripting/scriptengine/ScriptingDefines.h"

#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Job>

#include <QScriptContext>
#include <QScriptEngine>

Q_DECLARE_METATYPE( StringMap )

using namespace AmarokScript;

#define CHECK_TRACK( X )  if( !m_track ){ warning() << "Invalid track!"; return X; };
#define GET_TRACK_EC( X ) CHECK_TRACK() \
                          Meta::TrackEditorPtr ec = m_track->editor(); \
                          if( ec ) \
                          { \
                              X; \
                          }

void
MetaTrackPrototype::init( QScriptEngine *engine )
{
    qScriptRegisterMetaType<Meta::TrackPtr>( engine, toScriptValue<Meta::TrackPtr, MetaTrackPrototype>, fromScriptValue<Meta::TrackPtr, MetaTrackPrototype> );
    qScriptRegisterMetaType<Meta::TrackList>( engine, toScriptArray, fromScriptArray );
    qScriptRegisterMetaType<StringMap>( engine, toScriptMap, fromScriptMap );
    qScriptRegisterMetaType<Meta::FieldHash>( engine, toScriptTagMap, fromScriptTagMap );
    engine->globalObject().setProperty( "Track", engine->newFunction( trackCtor ) );
}

QScriptValue
MetaTrackPrototype::trackCtor( QScriptContext *context, QScriptEngine *engine )
{
    if( context->argumentCount() < 1 )
        return context->throwError( QScriptContext::SyntaxError, "Not enough arguments! Pass the track url." );

    QUrl url( qscriptvalue_cast<QUrl>( context->argument( 0 ) ) );
    if( !url.isValid() )
        return context->throwError( QScriptContext::TypeError, "Invalid QUrl" );

    MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( url ) );
    proxyTrack->setTitle( url.fileName() ); // set temporary name
    return engine->newQObject( new MetaTrackPrototype( Meta::TrackPtr( proxyTrack.data() ) )
                                                , QScriptEngine::ScriptOwnership
                                                , QScriptEngine::ExcludeSuperClassContents );
}

MetaTrackPrototype::MetaTrackPrototype( const Meta::TrackPtr &track )
: QObject( 0 )
, m_track( track )
{
}

Meta::FieldHash
MetaTrackPrototype::tags() const
{
    if( !isLoadedAndLocal() )
        return  Meta::FieldHash();

    return Meta::Tag::readTags( m_track->playableUrl().path() );
}

int
MetaTrackPrototype::sampleRate() const
{
    CHECK_TRACK( 0 )
    return m_track->sampleRate();
}

int
MetaTrackPrototype::bitrate() const
{
    CHECK_TRACK( 0 )
    return m_track->bitrate();
}

double
MetaTrackPrototype::score() const
{
    CHECK_TRACK( 0.0 )
    return m_track->statistics()->score();
}

int
MetaTrackPrototype::rating() const
{
    CHECK_TRACK( 0 )
    return m_track->statistics()->rating();
}

bool
MetaTrackPrototype::inCollection() const
{
    CHECK_TRACK( false )
    return m_track->inCollection();
}

QString
MetaTrackPrototype::type() const
{
    CHECK_TRACK( QString() )
    return m_track->type();
}

qint64
MetaTrackPrototype::length() const
{
    CHECK_TRACK( 0 )
    return m_track->length();
}

int
MetaTrackPrototype::fileSize() const
{
    CHECK_TRACK( 0 )
    return m_track->filesize();
}

int
MetaTrackPrototype::trackNumber() const
{
    CHECK_TRACK( 0 )
    return m_track->trackNumber();
}

int
MetaTrackPrototype::discNumber() const
{
    CHECK_TRACK( 0 )
    return m_track->discNumber();
}

int
MetaTrackPrototype::playCount() const
{
    CHECK_TRACK( 0 )
    return m_track->statistics()->playCount();
}

bool
MetaTrackPrototype::playable() const
{
    CHECK_TRACK( false )
    return m_track->isPlayable();
}

QString
MetaTrackPrototype::album() const
{
    CHECK_TRACK( QString() )
    return m_track->album() ? m_track->album()->prettyName() : QString();
}

QString
MetaTrackPrototype::artist() const
{
    CHECK_TRACK( QString() )
    return m_track->artist() ? m_track->artist()->prettyName() : QString();
}

QString
MetaTrackPrototype::composer() const
{
    CHECK_TRACK( QString() )
    return m_track->composer() ? m_track->composer()->prettyName() : QString();
}

QString
MetaTrackPrototype::genre() const
{
    CHECK_TRACK( QString() )
    return m_track->genre() ? m_track->genre()->prettyName() : QString();
}

int
MetaTrackPrototype::year() const
{
    CHECK_TRACK( 0 )
    return m_track->year() ? m_track->year()->year() : 0;
}

QString
MetaTrackPrototype::comment() const
{
    CHECK_TRACK( QString() )
    return m_track->comment();
}

QString
MetaTrackPrototype::path() const
{
    CHECK_TRACK( QString() )
    return m_track->playableUrl().path();
}

QString
MetaTrackPrototype::title() const
{
    CHECK_TRACK ( QString() )
    return m_track->prettyName();
}

QString
MetaTrackPrototype::imageUrl() const
{
    CHECK_TRACK( QString() )
    return m_track->album() ? m_track->album()->imageLocation().toDisplayString() : QString();
}

QString
MetaTrackPrototype::url() const
{
    CHECK_TRACK( QString() )
    return m_track->playableUrl().url();
}

double
MetaTrackPrototype::bpm() const
{
    CHECK_TRACK( 0.0 )
    return m_track->bpm();
}

bool
MetaTrackPrototype::isLoaded() const
{
    MetaProxy::TrackPtr proxyTrack = MetaProxy::TrackPtr::dynamicCast( m_track );
    if( proxyTrack && !proxyTrack->isResolved() )
    {
        const_cast<MetaTrackPrototype*>( this )->Observer::subscribeTo( m_track );
        return false;
    }
    return true;
}

QScriptValue
MetaTrackPrototype::imagePixmap( int size ) const
{
    CHECK_TRACK( QScriptValue() )
    return m_track->album() ? m_engine->toScriptValue( m_track->album()->image( size ) ) : QScriptValue();
}

bool
MetaTrackPrototype::isValid() const
{
    return m_track;
}

bool
MetaTrackPrototype::isEditable()
{
    CHECK_TRACK( false )
    return m_track->editor(); // converts to bool nicely
}

QString
MetaTrackPrototype::lyrics() const
{
    CHECK_TRACK( QString() )
    return m_track->cachedLyrics();
}

void
MetaTrackPrototype::setScore( double score )
{
    CHECK_TRACK()
    m_track->statistics()->setScore( score );
}

void
MetaTrackPrototype::setRating( int rating )
{
    CHECK_TRACK()
    m_track->statistics()->setRating( rating );
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
    CHECK_TRACK()
    m_track->setCachedLyrics( lyrics );
}

void
MetaTrackPrototype::setTitle( const QString& title )
{
    GET_TRACK_EC( ec->setTitle( title ) )
}

void
MetaTrackPrototype::setImageUrl( const QString& imageUrl )
{
    CHECK_TRACK()
    if( m_track->album() )
        m_track->album()->setImage( QImage(imageUrl) );
}

void
MetaTrackPrototype::metadataChanged( Meta::TrackPtr track )
{
    Observer::unsubscribeFrom( track );
    debug() << "Loaded track: " << track->prettyName();
    emit loaded( track );
}

void
MetaTrackPrototype::fromScriptTagMap( const QScriptValue &value, Meta::FieldHash &map )
{
    QScriptValueIterator it( value );
    while( it.hasNext() )
    {
        it.next();
        map[Meta::fieldForName( it.name() )] = it.value().toVariant();
    }
}

QScriptValue
MetaTrackPrototype::toScriptTagMap( QScriptEngine *engine, const Meta::FieldHash &map )
{
    QScriptValue scriptMap = engine->newObject();
    for( typename Meta::FieldHash::const_iterator it( map.constBegin() ); it != map.constEnd(); ++it )
        scriptMap.setProperty( Meta::nameForField( it.key() ), engine->toScriptValue( it.value() ) );
    return scriptMap;
}

void
MetaTrackPrototype::changeTags( const Meta::FieldHash &changes, bool respectConfig )
{
    if( !isLoadedAndLocal() )
        return;

    if( changes.isEmpty() )
        return;
    WriteTagsJob *job = new WriteTagsJob( m_track->playableUrl().path(), changes, respectConfig );
    connect( job, &WriteTagsJob::done, job, &QObject::deleteLater );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );
}

QImage
MetaTrackPrototype::embeddedCover() const
{
    if( isLoadedAndLocal() )
        return QImage();

    return Meta::Tag::embeddedCover( m_track->playableUrl().path() );
}

void
MetaTrackPrototype::setEmbeddedCover( const QImage &image )
{
    if( image.isNull() )
        return;
}

bool
MetaTrackPrototype::isLoadedAndLocal() const
{
    CHECK_TRACK( false );
    if( !isLoaded() )
    {
        debug() << "Track for url " << m_track->prettyUrl() << " not loaded yet!";
        return false;
    }
    if( !m_track->playableUrl().isLocalFile() )
    {
        debug() << m_track->prettyName() + " is not a local file!";
        return false;
    }

    return true;
}

#undef GET_TRACK
#undef CHECK_TRACK
#undef GET_TRACK_EC

