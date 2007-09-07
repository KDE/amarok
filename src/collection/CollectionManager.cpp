/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#define DEBUG_PREFIX "CollectionManager"

#include "CollectionManager.h"

#include "debug.h"

#include "BlockingQuery.h"
#include "Collection.h"
#include "MetaQueryBuilder.h"
#include "meta/file/File.h"
#include "meta/stream/Stream.h"
#include "meta/lastfm/LastFmMeta.h"
#include "pluginmanager.h"
#include "scrobbler.h"
#include "SqlStorage.h"

#include <QtAlgorithms>
#include <QtGlobal>
#include <QList>
#include <QTimer>

#include <kservice.h>

struct CollectionManager::Private
{
    QList<Collection*> collections;
    QList<CollectionFactory*> factories;
    SqlStorage *sqlDatabase;
    QList<Collection*> unmanagedCollections;
    QList<Collection*> managedCollections;
};

class CollectionManagerSingleton
{
    public:
        CollectionManager instance;
};

K_GLOBAL_STATIC( CollectionManagerSingleton, privateInstance )


CollectionManager *
CollectionManager::instance( )
{
    return &privateInstance->instance;
}

CollectionManager::CollectionManager()
    : QObject()
    , d( new Private )
{
    init();
}

CollectionManager::~CollectionManager()
{
    d->collections.clear();
    d->unmanagedCollections.clear();
    qDeleteAll( d->managedCollections );

    foreach( CollectionFactory *fac, d->factories )
        PluginManager::unload( fac );

    delete d;
}

void
CollectionManager::init()
{
    DEBUG_BLOCK

    d->sqlDatabase = 0;
    KService::List plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'collection'" );
    debug() << "Received [" << QString::number( plugins.count() ) << "] collection plugin offers";
    foreach( KService::Ptr service, plugins )
    {
        Amarok::Plugin *plugin = PluginManager::createFromService( service );
        if ( plugin )
        {
            CollectionFactory* factory = dynamic_cast<CollectionFactory*>( plugin );
            if ( factory )
            {
                connect( factory, SIGNAL( newCollection( Collection* ) ), this, SLOT( slotNewCollection( Collection* ) ) );
                d->factories.append( factory );
                factory->init();
            }
            else
            {
                debug() << "Plugin has wrong factory class";
                continue;
            }
        }
    }
}

void
CollectionManager::startFullScan()
{
    foreach( Collection *coll, d->collections )
    {
        coll->startFullScan();
    }
}

QueryMaker*
CollectionManager::queryMaker()
{
    return new MetaQueryBuilder( d->collections );
}

void
CollectionManager::slotNewCollection( Collection* newCollection )
{
    if( !newCollection )
    {
        debug() << "Warning, newCollection in slotNewCollection is 0";
        return;
    }
    debug() << "New collection with collectionId: " << newCollection->collectionId();
    d->collections.append( newCollection );
    d->managedCollections.append( newCollection );
    connect( newCollection, SIGNAL( remove() ), SLOT( slotRemoveCollection() ), Qt::QueuedConnection );
    SqlStorage *sqlCollection = dynamic_cast<SqlStorage*>( newCollection );
    if( sqlCollection )
    {
        if( d->sqlDatabase )
        {
            if( d->sqlDatabase->sqlDatabasePriority() < sqlCollection->sqlDatabasePriority() )
                d->sqlDatabase = sqlCollection;
        }
        else
            d->sqlDatabase = sqlCollection;
    }
    emit collectionAdded( newCollection );
}

void
CollectionManager::slotRemoveCollection()
{
    Collection* collection = dynamic_cast<Collection*>( sender() );
    if( collection )
    {
        d->collections.removeAll( collection );
        d->managedCollections.removeAll( collection );
        SqlStorage *sqlDb = dynamic_cast<SqlStorage*>( collection );
        if( sqlDb && sqlDb == d->sqlDatabase )
        {
            SqlStorage *newSqlDatabase = 0;
            foreach( Collection* tmp, d->collections )
            {
                SqlStorage *sqlDb = dynamic_cast<SqlStorage*>( tmp );
                if( sqlDb )
                {
                    if( newSqlDatabase )
                    {
                        if( newSqlDatabase->sqlDatabasePriority() < sqlDb->sqlDatabasePriority() )
                            newSqlDatabase = sqlDb;
                    }
                    else
                        newSqlDatabase = sqlDb;
                }
            }
            d->sqlDatabase = newSqlDatabase;
        }
        emit collectionRemoved( collection->collectionId() );
        QTimer::singleShot( 0, collection, SLOT( deleteLater() ) );
    }
}

QList<Collection*>
CollectionManager::collections()
{
    return d->collections;
}

SqlStorage*
CollectionManager::sqlStorage() const
{
    return d->sqlDatabase;
}

Meta::TrackList
CollectionManager::tracksForUrls( const KUrl::List &urls )
{
    Meta::TrackList tracks;
    foreach( KUrl url, urls )
        tracks.append( trackForUrl( url ) );
    return tracks;
}

Meta::TrackPtr
CollectionManager::trackForUrl( const KUrl &url )
{
    //TODO method stub
    //check all collections
    //might be a podcast, in that case we'll have additional meta information
    //might be a lastfm track, another stream
    //or a file which is not in any collection
    debug() << "track for url: " << url.url();
    //check lastfm track
    if( url.protocol() == "lastfm" )
        return Meta::TrackPtr( new LastFm::Track( url.url() ) );

    foreach( Collection *coll, d->collections )
    {
        if( coll->possiblyContainsTrack( url ) )
        {
            Meta::TrackPtr track = coll->trackForUrl( url );
            if( track )
                return track;
        }
    }

    if( url.protocol() == "http" || url.protocol() == "mms" )
        return Meta::TrackPtr( new MetaStream::Track( url ) );

    if( url.protocol() == "file" )
        return Meta::TrackPtr( new MetaFile::Track( url ) );

    return Meta::TrackPtr( 0 );
}

Meta::ArtistList
CollectionManager::relatedArtists( Meta::ArtistPtr artist, int maxArtists )
{
    SqlStorage *sql = sqlStorage();
    QString query = QString( "SELECT suggestion FROM related_artists WHERE artist = '%1' ORDER BY %2 LIMIT %3 OFFSET 0;" )
               .arg( sql->escape( artist->name() ), sql->randomFunc(), QString::number( maxArtists ) );

    QStringList artistNames = sql->query( query );
    //TODO: figure out a way to retrieve similar artists from last.fm here
    /*if( artistNames.isEmpty() )
    {
        artistNames = Scrobbler::instance()->similarArtists( artist->name() );
    }*/
    QueryMaker *qm = queryMaker();
    foreach( QString artist, artistNames )
    {
        qm->addFilter( QueryMaker::valArtist, artist, true, true );
    }
    qm->startArtistQuery();
    qm->limitMaxResultSize( maxArtists );
    BlockingQuery bq( qm );
    bq.startQuery();

    QStringList ids = bq.collectionIds();
    Meta::ArtistList result;
    QSet<QString> artistNameSet;
    foreach( QString collectionId, ids )
    {
        Meta::ArtistList artists = bq.artists( collectionId );
        foreach( Meta::ArtistPtr artist, artists )
        {
            if( !artistNameSet.contains( artist->name() ) )
            {
                result.append( artist );
                artistNameSet.insert( artist->name() );
                if( result.size() == maxArtists )
                    break;
            }
        }
        if( result.size() == maxArtists )
                    break;
    }
    return result;
}

void
CollectionManager::addUnmanagedCollection( Collection *newCollection )
{
    if( newCollection && d->collections.indexOf( newCollection ) == -1 )
    {
        d->unmanagedCollections.append( newCollection );
        d->collections.append( newCollection );
        emit collectionAdded( newCollection );
    }
}

void
CollectionManager::removeUnmanagedCollection( Collection *collection )
{
    //do not remove it from collection if it is not in unmanagedCollections
    if( collection && d->unmanagedCollections.removeAll( collection ) )
    {
        d->collections.removeAll( collection );
        emit collectionRemoved( collection->collectionId() );
    }
}

#include "CollectionManager.moc"
