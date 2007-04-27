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

#include "collectionmanager.h"

#include "debug.h"

#include "collection.h"
#include "metaquerybuilder.h"
#include "pluginmanager.h"

#include <QtGlobal>
#include <QList>

#include <kservice.h>

struct CollectionManager::Private
{
    QList<Collection*> collections;
    QList<CollectionFactory*> factories;
    Collection *sqlDatabase;
};

CollectionManager::CollectionManager()
    : QObject()
    , d( new Private )
{
    init();
}

CollectionManager::~CollectionManager()
{
    foreach( Collection *coll, d->collections )
        delete coll;

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
    debug() << "Received [" << QString::number( plugins.count() ) << "] collection plugin offers" << endl;
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
                debug() << "Plugin has wrong factory class" << endl;
                continue;
            }
        }
    }
}

CollectionManager *
CollectionManager::instance( )
{
    static CollectionManager instance;
    return &instance;
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
        debug() << "Warning, newCollection in slotNewCollection is 0" << endl;
        return;
    }
    debug() << "New collection with collectionId: " << newCollection->collectionId() << endl;
    d->collections.append( newCollection );
    if( newCollection->isSqlDatabase() )
    {
        if( d->sqlDatabase )
            if( d->sqlDatabase->sqlDatabasePriority() < newCollection->sqlDatabasePriority() )
                d->sqlDatabase = newCollection;
        else
            d->sqlDatabase = newCollection;
    }
    emit collectionAdded( newCollection );
}

QList<Collection*>
CollectionManager::collections()
{
    return d->collections;
}

QStringList
CollectionManager::sqlQuery( const QString &query )
{
    return d->sqlDatabase->query( query );
}

int
CollectionManager::sqlInsert( const QString &statement, const QString &table )
{
    return d->sqlDatabase->insert( statement, table );
}

#include "collectionmanager.moc"
