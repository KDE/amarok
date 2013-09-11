/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#include "AmarokCollectionScript.h"

#include "amarokconfig.h"
#include "App.h"
#include "core/collections/support/SqlStorage.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/db/sql/SqlCollectionLocation.h"
#include "core/collections/QueryMaker.h"

#include <QScriptEngine>

#define SCRIPTING_DEPRECATED( x ) \
                                    AmarokScriptEngine *amarokScriptEngine = dynamic_cast<AmarokScriptEngine*>(parent()); \
                                    if( amarokScriptEngine ) amarokScriptEngine->invokableDeprecatedCall( x );

using namespace AmarokScript;

using Collections::Collection;
using Collections::CollectionList;
using Collections::QueryMaker;

AmarokCollectionScript::AmarokCollectionScript( AmarokScriptEngine *engine )
    : QObject( engine )
{
    QScriptValue scriptObject = engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents );
    //deprecate
    engine->setDeprecatedProperty( "Amarok", "Collection", scriptObject );

    engine->globalObject().property( "Amarok" ).setProperty( "CollectionManager", scriptObject );

    CollectionManager *instance = CollectionManager::instance();
    connect( instance, SIGNAL(collectionDataChanged(Collections::Collection*)),
            SIGNAL(collectionDataChanged(Collections::Collection*)) );
    connect( instance, SIGNAL(collectionAdded(Collections::Collection*,CollectionManager::CollectionStatus)),
             SIGNAL(collectionAdded(Collections::Collection*)) );
    connect( instance, SIGNAL(collectionRemoved(QString)), SIGNAL(collectionRemoved(QString)) );
}

int
AmarokCollectionScript::totalAlbums() const
{
    QStringList albums = query( "SELECT COUNT( id ) FROM albums;" );
    if( albums.size() < 1 )
        return 0;
    QString total = albums[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalArtists() const
{
    QStringList artists = query( "SELECT COUNT( id ) FROM artists;" );
    if( artists.size() < 1 )
        return 0;
    QString total = artists[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalComposers() const
{
    QStringList composers = query( "SELECT COUNT( id ) FROM composers;" );
    if( composers.size() < 1 )
        return 0;
    QString total = composers[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalGenres() const
{
    QStringList genres = query( "SELECT COUNT( id ) FROM genres;" );
    if( genres.size() < 1 )
        return 0;
    QString total = genres[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalTracks() const
{
    QStringList tracks = query( "SELECT COUNT( url ) FROM tracks;" );
    if( tracks.size() < 0 )
        return 0;
    QString total = tracks[0];
    int final = total.toInt();
    return final;
}

QStringList
AmarokCollectionScript::collectionLocation() const
{
    SCRIPTING_DEPRECATED( "collectionLocation" )
    Collections::CollectionLocation *location = CollectionManager::instance()->primaryCollection()->location();
    QStringList result = location->actualLocation();
    delete location;
    return result;
}

QStringList
AmarokCollectionScript::query( const QString& sql ) const
{
    return CollectionManager::instance()->sqlStorage()->query( sql );
}

QString
AmarokCollectionScript::escape( const QString& sql ) const
{
    return CollectionManager::instance()->sqlStorage()->escape( sql );
}

void
AmarokCollectionScript::scanCollection() const
{
    CollectionManager::instance()->startFullScan();
}

void
AmarokCollectionScript::scanCollectionChanges() const
{
    CollectionManager::instance()->checkCollectionChanges();
}

CollectionList
AmarokCollectionScript::queryableCollections() const
{
    return CollectionManager::instance()->queryableCollections();
}

QueryMaker*
AmarokCollectionScript::queryMaker() const
{
    return CollectionManager::instance()->queryMaker();
}

CollectionList
AmarokCollectionScript::viewableCollections() const
{
    return CollectionManager::instance()->viewableCollections();
}
