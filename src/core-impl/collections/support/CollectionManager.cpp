/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#define DEBUG_PREFIX "CollectionManager"

#include "CollectionManager.h"

#include "core/capabilities/CollectionScanCapability.h"
#include "core/collections/Collection.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/meta/file/FileTrackProvider.h"
#include "core-impl/meta/stream/Stream.h"
#include "core-impl/meta/timecode/TimecodeTrackProvider.h"

#include <QMetaEnum>
#include <QMetaObject>

#include <QCoreApplication>
#include <QList>
#include <QPair>
#include <QTimer>
#include <QReadWriteLock>


typedef QPair<Collections::Collection*, CollectionManager::CollectionStatus> CollectionPair;


/** Private structure of the collection manager */
struct CollectionManager::Private
{
    QList<CollectionPair> collections;
    QList<QSharedPointer<Plugins::PluginFactory> > factories; // factories belong to PluginManager

    QList<Collections::TrackProvider*> trackProviders;
    TimecodeTrackProvider *timecodeTrackProvider;
    Collections::TrackProvider *fileTrackProvider; // special case

    Collections::Collection *primaryCollection;

    QReadWriteLock lock; ///< protects all other variables against threading issues
};

CollectionManager *CollectionManager::s_instance = nullptr;

CollectionManager *
CollectionManager::instance()
{
    if( !s_instance ) {
        s_instance = new CollectionManager();
        s_instance->init();
    }

    return s_instance;
}

void
CollectionManager::destroy()
{
    if( s_instance ) {
        delete s_instance;
        s_instance = nullptr;
    }
}

CollectionManager::CollectionManager()
    : QObject()
    , d( new Private )
{
    DEBUG_BLOCK
    // ensure this object is created in a main thread
    Q_ASSERT( thread() == QCoreApplication::instance()->thread() );

    setObjectName( QStringLiteral("CollectionManager") );
    d->primaryCollection = nullptr;
    d->timecodeTrackProvider = nullptr;
    d->fileTrackProvider = nullptr;
}

CollectionManager::~CollectionManager()
{
    DEBUG_BLOCK

    {
        QWriteLocker locker( &d->lock );

        while (!d->collections.isEmpty() )
            delete d->collections.takeFirst().first;

        d->trackProviders.clear();
        delete d->timecodeTrackProvider;
        delete d->fileTrackProvider;
    }

    delete d;
}

void
CollectionManager::init()
{
    // register the timecode track provider now, as it needs to get added before loading
    // the stored playlist... Since it can have playable urls that might also match other providers, it needs to get added first.
    d->timecodeTrackProvider = new TimecodeTrackProvider();
    addTrackProvider( d->timecodeTrackProvider );

    // addint fileTrackProvider second since local tracks should be preferred even if the url matches two tracks
    d->fileTrackProvider = new FileTrackProvider();
    addTrackProvider( d->fileTrackProvider );
}

void
CollectionManager::setFactories( const QList<QSharedPointer<Plugins::PluginFactory> > &factories )
{
    using Collections::CollectionFactory;


    QSet<QSharedPointer<Plugins::PluginFactory> > newFactories(factories.begin(), factories.end());
    QSet<QSharedPointer<Plugins::PluginFactory> > oldFactories;

    {
        QReadLocker locker( &d->lock );
        QSet<QSharedPointer<Plugins::PluginFactory> > addFactoriesSet(d->factories.begin(), d->factories.end());
        oldFactories += addFactoriesSet;
    }

    // remove old factories
    for( const auto &pFactory : oldFactories - newFactories )
    {

        auto factory = qobject_cast<CollectionFactory*>( pFactory );
        if( !factory )
            continue;

        disconnect( factory.data(), &CollectionFactory::newCollection,
                    this, &CollectionManager::slotNewCollection );
        {
            QWriteLocker locker( &d->lock );
            d->factories.removeAll( factory );
        }
    }

    // create new factories
    for( const auto &pFactory : newFactories - oldFactories )
    {
        auto factory = qobject_cast<CollectionFactory*>( pFactory );
        if( !factory )
            continue;

        connect( factory.data(), &CollectionFactory::newCollection,
                 this, &CollectionManager::slotNewCollection );
        {
            QWriteLocker locker( &d->lock );
            d->factories.append( factory );
        }
    }
}


void
CollectionManager::startFullScan()
{
    QReadLocker locker( &d->lock );

    for( const CollectionPair &pair : d->collections )
    {
        QScopedPointer<Capabilities::CollectionScanCapability> csc( pair.first->create<Capabilities::CollectionScanCapability>() );
        if( csc )
            csc->startFullScan();
    }
}

void
CollectionManager::startIncrementalScan( const QString &directory )
{
    QReadLocker locker( &d->lock );

    for( const CollectionPair &pair : d->collections )
    {
        QScopedPointer<Capabilities::CollectionScanCapability> csc( pair.first->create<Capabilities::CollectionScanCapability>() );
        if( csc )
            csc->startIncrementalScan( directory );
    }
}

void
CollectionManager::stopScan()
{
    QReadLocker locker( &d->lock );

    for( const CollectionPair &pair : d->collections )
    {
        QScopedPointer<Capabilities::CollectionScanCapability> csc( pair.first->create<Capabilities::CollectionScanCapability>() );
        if( csc )
            csc->stopScan();
    }
}

void
CollectionManager::checkCollectionChanges()
{
    startIncrementalScan( QString() );
}

Collections::QueryMaker*
CollectionManager::queryMaker() const
{
    QReadLocker locker( &d->lock );

    QList<Collections::Collection*> colls;
    for( const CollectionPair &pair : d->collections )
    {
        if( pair.second & CollectionQueryable )
        {
            colls << pair.first;
        }
    }
    return new Collections::MetaQueryMaker( colls );
}

void
CollectionManager::slotNewCollection( Collections::Collection* newCollection )
{
    DEBUG_BLOCK

    if( !newCollection )
    {
        error() << "newCollection in slotNewCollection is 0";
        return;
    }
    {
        QWriteLocker locker( &d->lock );
        for( const CollectionPair &p : d->collections )
        {
            if( p.first == newCollection )
            {
                error() << "newCollection " << newCollection->collectionId() << " is already being managed";
                return;
            }
        }
    }

    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "CollectionStatus" ) );
    const QString &value = Amarok::config( QStringLiteral("CollectionManager") ).readEntry( newCollection->collectionId() );
    int enumValue = me.keyToValue( value.toLocal8Bit().constData() );
    CollectionStatus status;
    enumValue == -1 ? status = CollectionEnabled : status = (CollectionStatus) enumValue;
    CollectionPair pair( newCollection, status );

    {
        QWriteLocker locker( &d->lock );
        if( newCollection->collectionId() == QLatin1String("localCollection") )
        {
            d->primaryCollection = newCollection;
            d->collections.insert( 0, pair ); // the primary collection should be the first collection to be searched
            d->trackProviders.insert( 1, newCollection ); // the primary collection should be between the timecode track provider and the local file track provider
        }
        else
        {
            d->collections.append( pair );
            d->trackProviders.append( newCollection );
        }
        connect( newCollection, &Collections::Collection::remove, this, &CollectionManager::slotRemoveCollection, Qt::QueuedConnection );
        connect( newCollection, &Collections::Collection::updated, this, &CollectionManager::slotCollectionChanged, Qt::QueuedConnection );

        debug() << "new Collection " << newCollection->collectionId();
    }

    if( status & CollectionViewable )
    {
//         Q_EMIT collectionAdded( newCollection );
        Q_EMIT collectionAdded( newCollection, status );
    }
}

void
CollectionManager::slotRemoveCollection()
{
    Collections::Collection* collection = qobject_cast<Collections::Collection*>( sender() );
    if( collection )
    {
        CollectionStatus status = collectionStatus( collection->collectionId() );
        CollectionPair pair( collection, status );

        {
            QWriteLocker locker( &d->lock );
            d->collections.removeAll( pair );
            d->trackProviders.removeAll( collection );
        }

        Q_EMIT collectionRemoved( collection->collectionId() );
        QTimer::singleShot( 500, collection, &QObject::deleteLater ); // give the tree some time to update itself until we really delete the collection pointers.
    }
}

void
CollectionManager::slotCollectionChanged()
{
    Collections::Collection *collection = dynamic_cast<Collections::Collection*>( sender() );
    if( collection )
    {
        CollectionStatus status = collectionStatus( collection->collectionId() );
        if( status & CollectionViewable )
        {
            Q_EMIT collectionDataChanged( collection );
        }
    }
}

QList<Collections::Collection*>
CollectionManager::viewableCollections() const
{
    QReadLocker locker( &d->lock );

    QList<Collections::Collection*> result;
    for( const CollectionPair &pair : d->collections )
    {
        if( pair.second & CollectionViewable )
        {
            result << pair.first;
        }
    }
    return result;
}

Collections::Collection*
CollectionManager::primaryCollection() const
{
    QReadLocker locker( &d->lock );

    return d->primaryCollection;
}

Meta::TrackPtr
CollectionManager::trackForUrl( const QUrl &url )
{
    QReadLocker locker( &d->lock );

    // TODO:
    // might be a podcast, in that case we'll have additional meta information
    // might be a lastfm track, another stream
    if( !url.isValid() )
        return Meta::TrackPtr( nullptr );

    for( Collections::TrackProvider *provider : d->trackProviders )
    {
        if( provider->possiblyContainsTrack( url ) )
        {
            Meta::TrackPtr track = provider->trackForUrl( url );
            if( track )
                return track;
        }
    }

    // TODO: create specific TrackProviders for these:
    static const QSet<QString> remoteProtocols = QSet<QString>()
            << QStringLiteral("http") << QStringLiteral("https") << QStringLiteral("mms") << QStringLiteral("smb"); // consider unifying with TrackLoader::tracksLoaded()
    if( remoteProtocols.contains( url.scheme() ) )
        return Meta::TrackPtr( new MetaStream::Track( url ) );

    return Meta::TrackPtr( nullptr );
}


CollectionManager::CollectionStatus
CollectionManager::collectionStatus( const QString &collectionId ) const
{
    QReadLocker locker( &d->lock );

    for( const CollectionPair &pair : d->collections )
    {
        if( pair.first->collectionId() == collectionId )
        {
            return pair.second;
        }
    }
    return CollectionDisabled;
}

QHash<Collections::Collection*, CollectionManager::CollectionStatus>
CollectionManager::collections() const
{
    QReadLocker locker( &d->lock );

    QHash<Collections::Collection*, CollectionManager::CollectionStatus> result;
    for( const CollectionPair &pair : d->collections )
    {
        result.insert( pair.first, pair.second );
    }
    return result;
}

void
CollectionManager::addTrackProvider( Collections::TrackProvider *provider )
{
    {
        QWriteLocker locker( &d->lock );
        d->trackProviders.append( provider );
    }
    Q_EMIT trackProviderAdded( provider );
}

void
CollectionManager::removeTrackProvider( Collections::TrackProvider *provider )
{
    QWriteLocker locker( &d->lock );
    d->trackProviders.removeAll( provider );
}

