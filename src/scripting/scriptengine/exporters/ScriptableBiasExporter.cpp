/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#define DEBUG_PREFIX "ScriptableBias"

#include "ScriptableBiasExporter.h"

#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QJSEngine>
#include <QQmlEngine>
#include <QThread>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace AmarokScript;

ScriptableBiasFactoryWrapper::ScriptableBiasFactoryWrapper(QJSEngine *engine)
: QObject( engine )
, m_engine( engine )
{
}

QJSValue
ScriptableBiasFactoryWrapper::biasCtor()
{
    return m_engine->newQObject( new ScriptableBiasFactory( m_engine ) );
}

QJSValue
ScriptableBiasFactoryWrapper::groupBiasCtor()
{
    return m_engine->newQObject( new ScriptableBiasFactory( m_engine, true ) );
}

void
ScriptableBiasFactory::init( QJSEngine *engine )
{
    TrackSetExporter::init( engine );
    QJSValue scriptObj = engine->newQObject( new ScriptableBiasFactoryWrapper( engine ) );
    engine->globalObject().setProperty( QStringLiteral("BiasFactory"),  scriptObj.property(QStringLiteral("biasCtor")) );
    engine->globalObject().setProperty( QStringLiteral("GroupBiasFactory"), scriptObj.property(QStringLiteral("groupBiasCtor")) );
}

ScriptableBiasFactory::ScriptableBiasFactory( QJSEngine *engine, bool groupBias )
: QObject( engine )
, m_groupBias( groupBias )
, m_engine( engine )
, m_enabled( false )
{}

ScriptableBiasFactory::~ScriptableBiasFactory()
{
    Dynamic::BiasFactory::instance()->removeBiasFactory( this );
}

Dynamic::BiasPtr
ScriptableBiasFactory::createBias()
{
    ScriptableBias *bias;
    //if( m_groupBias )
    //    return new ScriptableGroupBias( this );
    //else
    bias = new ScriptableBias( this );
    Dynamic::BiasPtr biasPtr = Dynamic::BiasPtr( bias );
    QJSValue biasObject = bias->scriptObject();
    if( m_initFunction.isCallable() )
        m_initFunction.callWithInstance( biasObject, QJSValueList() << biasObject );

    return biasPtr;
}

QJSEngine*
ScriptableBiasFactory::engine() const
{
    return m_engine;
}

void
ScriptableBiasFactory::setEnabled( bool enabled )
{
    if( enabled )
    {
        if( !m_enabled )
            Dynamic::BiasFactory::instance()->registerNewBiasFactory( this );
    }
    else
        Dynamic::BiasFactory::instance()->removeBiasFactory( this );
    m_enabled = enabled;
}

bool
ScriptableBiasFactory::enabled() const
{
    return m_enabled;
}

void
ScriptableBiasFactory::setName( const QString &name )
{
    m_name = name;
}

QString
ScriptableBiasFactory::name() const
{
    return m_name;
}

QString
ScriptableBiasFactory::i18nName() const
{
    return m_i18nName;
}

QString
ScriptableBiasFactory::i18nDescription() const
{
    return m_description;
}

QJSValue
ScriptableBiasFactory::initFunction() const
{
    return m_initFunction;
}

void
ScriptableBiasFactory::setInitFunction( const QJSValue &value )
{
    m_initFunction = value;
}

void
ScriptableBiasFactory::setI18nDescription( const QString &description )
{
    m_description = description;
}

void
ScriptableBiasFactory::setI18nName( const QString &i18nName )
{
    m_i18nName = i18nName;
}

QJSValue
ScriptableBiasFactory::widgetFunction() const
{
    return m_widgetFunction;
}

void
ScriptableBiasFactory::setWidgetFunction( const QJSValue &value )
{
    // throw exception?
    //if( !value.isFunction() )
    m_widgetFunction = value;
}

void
ScriptableBiasFactory::setFromXmlFunction( const QJSValue &value )
{
    m_fromXmlFunction = value;
}

void
ScriptableBiasFactory::setToXmlFunction( const QJSValue &value )
{
    m_toXmlFunction = value;
}

QJSValue
ScriptableBiasFactory::fromXmlFunction() const
{
    return m_fromXmlFunction;
}

QJSValue
ScriptableBiasFactory::toXmlFunction() const
{
    return m_toXmlFunction;
}

QJSValue
ScriptableBiasFactory::matchingTracksFunction() const
{
    return m_matchingTracksFunction;
}

void
ScriptableBiasFactory::setMatchingTracksFunction( const QJSValue &value )
{
    m_matchingTracksFunction = value;
}

void
ScriptableBiasFactory::setTrackMatchesFunction( const QJSValue &value )
{
    m_trackMatchesFunction = value;
}

QJSValue
ScriptableBiasFactory::trackMatchesFunction() const
{
    return m_trackMatchesFunction;
}

void
ScriptableBiasFactory::setToStringFunction( const QJSValue &value )
{
    m_toStringFunction = value;
}

QJSValue
ScriptableBiasFactory::toStringFunction() const
{
    return m_toStringFunction;
}

/*********************************************************************************
// ScriptableBias
**********************************************************************************/
void
ScriptableBias::toXml( QXmlStreamWriter *writer ) const
{
    if( m_scriptBias->toXmlFunction().isCallable() )
        m_scriptBias->fromXmlFunction().callWithInstance( m_biasObject,
                                              QJSValueList() << m_engine->toScriptValue<QXmlStreamWriter*>( writer ) );
    else
        Dynamic::AbstractBias::toXml( writer );
}

void
ScriptableBias::fromXml( QXmlStreamReader *reader )
{
    if( m_scriptBias->fromXmlFunction().isCallable() )
        m_scriptBias->fromXmlFunction().callWithInstance( m_biasObject,
                                              QJSValueList() << m_engine->toScriptValue<QXmlStreamReader*>( reader ) );
    else
        Dynamic::AbstractBias::fromXml( reader );
}

QWidget*
ScriptableBias::widget( QWidget *parent )
{
    QWidget *widget = dynamic_cast<QWidget*>( m_scriptBias->widgetFunction().callWithInstance( m_biasObject,
                                                                                  QJSValueList() << m_scriptBias->engine()->newQObject( parent ) ).toQObject() );
    if( widget )
        return widget;
    return Dynamic::AbstractBias::widget( parent );
}

void
ScriptableBias::invalidate()
{
    Dynamic::AbstractBias::invalidate();
}

Dynamic::TrackSet
ScriptableBias::matchingTracks(const Meta::TrackList &playlist, int contextCount, int finalCount, const Dynamic::TrackCollectionPtr &universe ) const
{
    DEBUG_BLOCK
    if( QThread::currentThread() == QCoreApplication::instance()->thread() )
        return slotMatchingTracks( playlist, contextCount, finalCount, universe );

    Dynamic::TrackSet retVal;
    Q_ASSERT( QMetaObject::invokeMethod( const_cast<ScriptableBias*>( this ), "slotMatchingTracks", Qt::BlockingQueuedConnection,
                                         Q_RETURN_ARG( Dynamic::TrackSet, retVal),
                                         Q_ARG( Meta::TrackList, playlist ),
                                         Q_ARG( int, contextCount ),
                                         Q_ARG( int, finalCount ),
                                         Q_ARG( Dynamic::TrackCollectionPtr, universe )
                                       ) );
    debug() << "Returning trackSet, trackCount " << retVal.trackCount() << ", isOutstanding " << retVal.isOutstanding();
    return retVal;
}

Dynamic::TrackSet
ScriptableBias::slotMatchingTracks( const Meta::TrackList &playlist, int contextCount, int finalCount, const Dynamic::TrackCollectionPtr &universe ) const
{
    Q_ASSERT( QThread::currentThread() == QCoreApplication::instance()->thread() );
    if( m_scriptBias->matchingTracksFunction().isCallable() )
    {
        QJSValue trackSetVal = m_scriptBias->matchingTracksFunction().callWithInstance( m_biasObject,
                                                                                QJSValueList() << m_engine->toScriptValue<Meta::TrackList>( playlist )
                                                                                                   << contextCount
                                                                                                   << finalCount
                                                                                                   << m_engine->toScriptValue<QStringList>( universe->uids() ) );
        TrackSetExporter *trackSetExporter = dynamic_cast<TrackSetExporter*>( trackSetVal.toQObject() );
        if( trackSetExporter )
            return Dynamic::TrackSet( *trackSetExporter );
    }
    debug() << "Invalid trackSet received";
    return Dynamic::TrackSet( universe, false );
}

QString
ScriptableBias::name() const
{
    QString name;
    if( m_scriptBias )
        name = m_scriptBias->name();
    return name.isEmpty() ? Dynamic::AbstractBias::name() : name;
}

void
ScriptableBias::ready( const Dynamic::TrackSet &trackSet )
{
    debug() << "Received trackset, count: " << trackSet.trackCount() << "Is outstanding:" << trackSet.isOutstanding();
    Q_EMIT resultReady( trackSet );
}

void
ScriptableBias::paintOperator( QPainter *painter, const QRect &rect, Dynamic::AbstractBias *bias )
{
    Dynamic::AbstractBias::paintOperator( painter, rect, bias );
}

void
ScriptableBias::replace(const Dynamic::BiasPtr &newBias )
{
    Dynamic::AbstractBias::replace( newBias );
}

QString
ScriptableBias::toString() const
{
    return m_scriptBias->toStringFunction().call( QJSValueList() << m_biasObject ).toString();
}

bool
ScriptableBias::trackMatches( int position, const Meta::TrackList& playlist, int contextCount ) const
{
    if( m_scriptBias->trackMatchesFunction().isCallable() )
        return m_scriptBias->trackMatchesFunction().callWithInstance( m_biasObject,
                                                          QJSValueList() << position
                                                                             << m_engine->toScriptValue<Meta::TrackList>( playlist )
                                                                             << contextCount
                                                        ).toBool();
    return true;
}

ScriptableBias::ScriptableBias( ScriptableBiasFactory *biasProto )
: m_scriptBias( biasProto )
, m_engine( biasProto->engine() )
{
    m_biasObject = m_engine->newQObject( this );
    QQmlEngine::setObjectOwnership( this, QQmlEngine::CppOwnership );
    connect( m_engine, &QObject::destroyed, this, &ScriptableBias::removeBias );
}

ScriptableBias::~ScriptableBias()
{}

void
ScriptableBias::removeBias()
{
    replace( Dynamic::BiasPtr( new Dynamic::ReplacementBias( name() ) ) );
}

/////////////////////////////////////////////////////////////////////////////////////////
// TrackSetExporterWrapper
/////////////////////////////////////////////////////////////////////////////////////////

TrackSetExporterWrapper::TrackSetExporterWrapper( QJSEngine* engine )
: QObject( engine )
, m_engine (engine )
 {
}
QJSValue
TrackSetExporterWrapper::trackSetConstructor( QJSValue arg0, QJSValue arg1 )
{
    DEBUG_BLOCK

    TrackSetExporter *trackSetExporter = nullptr;
    // Called with 1 argument
    if ( !arg0.isUndefined() && arg1.isUndefined() )
    {
        TrackSetExporter *trackSetPrototype = dynamic_cast<TrackSetExporter*>( arg0.toQObject() );
        if( trackSetPrototype ) {
            trackSetExporter = new TrackSetExporter( Dynamic::TrackSet( *trackSetPrototype ) );
        }
    // Called with 2 arguments
    } else if ( !arg0.isUndefined() && arg1.isBool() ) {
        bool isFull = arg1.toBool();
        QStringList uidList;
        Meta::TrackList trackList;
        if( arg0.toVariant().canConvert<QStringList>() )
        {
            uidList = arg0.toVariant().toStringList();
            Q_ASSERT( !arg0.toVariant().canConvert<Meta::TrackList>() );
            trackSetExporter =  new TrackSetExporter( Dynamic::TrackSet( Dynamic::TrackCollectionPtr( new Dynamic::TrackCollection( uidList ) ), isFull ) );
        }
        else if( arg0.toVariant().canConvert<Meta::TrackList>() )
        {
            debug() << "In Meta::Tracklist TrackSet ctor";
            trackList = qjsvalue_cast<Meta::TrackList>( arg0 );
            for( const Meta::TrackPtr &track : trackList )
            {
                if( track )
                    uidList << track->uidUrl();
            }
            trackSetExporter = new TrackSetExporter( Dynamic::TrackSet( Dynamic::TrackCollectionPtr( new Dynamic::TrackCollection( uidList ) ), isFull  ) );
        }
    }
    if( trackSetExporter == nullptr )
    {
        m_engine->throwError( QJSValue::SyntaxError, QStringLiteral("Invalid arguments for TrackSet!") );
        return QJSValue(QJSValue::UndefinedValue);
    }

    const QJSValue trackSetObject = m_engine->newQObject( trackSetExporter );
    return trackSetObject;
}

/////////////////////////////////////////////////////////////////////////////////////////
// TrackSetExporter
/////////////////////////////////////////////////////////////////////////////////////////

void
TrackSetExporter::init( QJSEngine *engine )
{
    qRegisterMetaType<Dynamic::TrackSet>();
    QMetaType::registerConverter<Dynamic::TrackSet, QJSValue>( [=] (Dynamic::TrackSet trackSet) { return toScriptValue( engine, trackSet ); } );
    QMetaType::registerConverter<QJSValue, Dynamic::TrackSet>( [] (QJSValue jsValue) {
        Dynamic::TrackSet trackSet;
        fromScriptValue( jsValue, trackSet );
        return trackSet;
    } );
    engine->globalObject().setProperty( QStringLiteral("TrackSet"), engine->newQObject( new TrackSetExporterWrapper( engine ) ).property( "trackSetConstructor" ) );
}

void
TrackSetExporter::fromScriptValue( const QJSValue &obj, Dynamic::TrackSet &trackSet )
{
    DEBUG_BLOCK
    TrackSetExporter *trackSetProto = dynamic_cast<TrackSetExporter*>( obj.toQObject() );
    if( !trackSetProto )
        trackSet = Dynamic::TrackSet( Dynamic::TrackCollectionPtr( new Dynamic::TrackCollection( QStringList() ) ), false );
    else
        trackSet = *trackSetProto;
}

QJSValue
TrackSetExporter::toScriptValue( QJSEngine *engine, const Dynamic::TrackSet &trackSet )
{
    DEBUG_BLOCK
    TrackSetExporter *trackProto = new TrackSetExporter( trackSet );
    QJSValue val = engine->newQObject( trackProto );
    QQmlEngine::setObjectOwnership( trackProto, QQmlEngine::JavaScriptOwnership);
    return val;
}

bool
TrackSetExporter::containsUid( const QString &uid ) const
{
    return Dynamic::TrackSet::contains( uid );
}


void
TrackSetExporter::reset( bool value )
{
    Dynamic::TrackSet::reset( value );
}

void
TrackSetExporter::intersectTrackSet( const Dynamic::TrackSet &trackSet)
{
    Dynamic::TrackSet::intersect( trackSet );
}

void
TrackSetExporter::intersectUids( const QStringList &uids )
{
    Dynamic::TrackSet::intersect( uids );
}

void
TrackSetExporter::subtractTrack( const Meta::TrackPtr &track )
{
    Dynamic::TrackSet::subtract( track );
}

void
TrackSetExporter::subtractTrackSet( const Dynamic::TrackSet &trackSet )
{
    Dynamic::TrackSet::subtract( trackSet );
}

void
TrackSetExporter::subtractUids( const QStringList &uids )
{
    Dynamic::TrackSet::subtract( uids );
}

void
TrackSetExporter::uniteTrack( const Meta::TrackPtr &track )
{
    Dynamic::TrackSet::unite( track );
}

void
TrackSetExporter::uniteTrackSet( const Dynamic::TrackSet &trackSet )
{
    Dynamic::TrackSet::unite( trackSet );
}

void
TrackSetExporter::uniteUids( const QStringList &uids )
{
    Dynamic::TrackSet::unite( uids );
}

Meta::TrackPtr
TrackSetExporter::getRandomTrack() const
{
    return CollectionManager::instance()->trackForUrl( QUrl( Dynamic::TrackSet::getRandomTrack() ) );
}

bool
TrackSetExporter::containsTrack( const Meta::TrackPtr &track ) const
{
    return Dynamic::TrackSet::contains( track );
}

// private
TrackSetExporter::TrackSetExporter( const Dynamic::TrackSet &trackSet )
: QObject( nullptr )
, TrackSet( trackSet )
{}
