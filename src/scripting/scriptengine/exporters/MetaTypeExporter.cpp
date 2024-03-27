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

#include <QJSEngine>

Q_DECLARE_METATYPE( StringMap )

using namespace AmarokScript;

#define CHECK_TRACK( X )  if( !m_track ){ warning() << "Invalid track!"; return X; };
#define GET_TRACK_EC( X ) CHECK_TRACK() \
                          Meta::TrackEditorPtr ec = m_track->editor(); \
                          if( ec ) \
                          { \
                              X; \
                          }

MetaTrackPrototypeWrapper::MetaTrackPrototypeWrapper( QJSEngine *engine )
: QObject( engine )
, m_engine( engine )
{
}

QJSValue
MetaTrackPrototypeWrapper::trackCtor( QJSValue arg )
{
    QUrl url( qjsvalue_cast<QUrl>( arg ) );
    if( !url.isValid() ) {
        QJSValue errorObj = m_engine->newErrorObject( QJSValue::TypeError, QStringLiteral("Invalid QUrl") );
        m_engine->throwError( errorObj.errorType(), errorObj.toString() );
        return errorObj;
    }

    MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( url ) );
    proxyTrack->setTitle( url.fileName() ); // set temporary name
    return m_engine->newQObject( new MetaTrackPrototype( Meta::TrackPtr( proxyTrack.data() ) ) );
}

MetaTrackPrototypeWrapper *MetaTrackPrototype::s_wrapper = nullptr;

void
MetaTrackPrototype::init( QJSEngine *engine )
{
    qRegisterMetaType<Meta::TrackPtr>();
    QMetaType::registerConverter<Meta::TrackPtr, QJSValue>( [=] (Meta::TrackPtr trackPtr) { return toScriptValue<Meta::TrackPtr, MetaTrackPrototype>( engine, trackPtr ); } );
    QMetaType::registerConverter<QJSValue, Meta::TrackPtr>( [] (QJSValue jsValue) {
        Meta::TrackPtr trackPtr;
        fromScriptValue<Meta::TrackPtr, MetaTrackPrototype>( jsValue, trackPtr );
        return trackPtr;
    } );

    qRegisterMetaType<Meta::TrackList>();
    QMetaType::registerConverter<Meta::TrackList,QJSValue>( [=] (Meta::TrackList trackList) { return toScriptArray<Meta::TrackList>( engine, trackList); } );
    QMetaType::registerConverter<QJSValue,Meta::TrackList>( [] (QJSValue jsValue) {
        Meta::TrackList trackList;
        fromScriptArray<Meta::TrackList>( jsValue, trackList );
        return trackList;
    } );

    qRegisterMetaType<StringMap>();
    QMetaType::registerConverter<StringMap,QJSValue>( [=] (StringMap stringMap) { return toScriptMap<StringMap>( engine, stringMap); } );
    QMetaType::registerConverter<QJSValue,StringMap>( [] (QJSValue jsValue) {
        StringMap stringMap;
        fromScriptMap<StringMap>( jsValue, stringMap );
        return stringMap;
    } );

    qRegisterMetaType<Meta::FieldHash>();
    QMetaType::registerConverter<Meta::FieldHash,QJSValue>( [=] (Meta::FieldHash fieldHash) { return toScriptTagMap( engine, fieldHash); } );
    QMetaType::registerConverter<QJSValue,Meta::FieldHash>( [] (QJSValue jsValue) {
        Meta::FieldHash fieldHash;
        fromScriptTagMap( jsValue, fieldHash );
        return fieldHash;
    } );

    if (s_wrapper == nullptr)
        s_wrapper = new MetaTrackPrototypeWrapper( engine );
    QJSValue scriptObj = engine->newQObject( s_wrapper );

    engine->globalObject().setProperty( QStringLiteral("Track"),  scriptObj.property("trackCtor"));
}

MetaTrackPrototype::MetaTrackPrototype( const Meta::TrackPtr &track )
: QObject( nullptr )
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

QJSValue
MetaTrackPrototype::imagePixmap( int size ) const
{
    CHECK_TRACK( QJSValue() )
    return m_track->album() ? m_engine->toScriptValue( m_track->album()->image( size ) ) : QJSValue();
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
MetaTrackPrototype::metadataChanged(const Meta::TrackPtr &track )
{
    Observer::unsubscribeFrom( track );
    debug() << "Loaded track: " << track->prettyName();
    Q_EMIT loaded( track );
}

void
MetaTrackPrototype::fromScriptTagMap( const QJSValue &value, Meta::FieldHash &map )
{
    QJSValueIterator it( value );
    while( it.hasNext() )
    {
        it.next();
        map[Meta::fieldForName( it.name() )] = it.value().toVariant();
    }
}

QJSValue
MetaTrackPrototype::toScriptTagMap( QJSEngine *engine, const Meta::FieldHash &map )
{
    QJSValue scriptMap = engine->newObject();
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

