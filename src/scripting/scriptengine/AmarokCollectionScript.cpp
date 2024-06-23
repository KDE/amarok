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
#include <core/storage/SqlStorage.h>
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/storage/StorageManager.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/db/sql/SqlCollectionLocation.h"
#include "core/collections/QueryMaker.h"

#include <QJSEngine>

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
    QJSValue scriptObject = engine->newQObject( this );
    //deprecate
    engine->setDeprecatedProperty( QStringLiteral("Amarok"), QStringLiteral("Collection"), scriptObject );

    engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("CollectionManager"), scriptObject );

    CollectionManager *instance = CollectionManager::instance();
    connect( instance, &CollectionManager::collectionDataChanged,
             this, &AmarokCollectionScript::collectionDataChanged );
    connect( instance, &CollectionManager::collectionAdded,
             this, &AmarokCollectionScript::collectionAdded );
    connect( instance, &CollectionManager::collectionRemoved,
             this, &AmarokCollectionScript::collectionRemoved );
}

int
AmarokCollectionScript::totalAlbums() const
{
    QStringList albums = query( QStringLiteral("SELECT COUNT( id ) FROM albums;") );
    if( albums.size() < 1 )
        return 0;
    QString total = albums[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalArtists() const
{
    QStringList artists = query( QStringLiteral("SELECT COUNT( id ) FROM artists;") );
    if( artists.size() < 1 )
        return 0;
    QString total = artists[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalComposers() const
{
    QStringList composers = query( QStringLiteral("SELECT COUNT( id ) FROM composers;") );
    if( composers.size() < 1 )
        return 0;
    QString total = composers[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalGenres() const
{
    QStringList genres = query( QStringLiteral("SELECT COUNT( id ) FROM genres;") );
    if( genres.size() < 1 )
        return 0;
    QString total = genres[0];
    return total.toInt();
}

int
AmarokCollectionScript::totalTracks() const
{
    QStringList tracks = query( QStringLiteral("SELECT COUNT( url ) FROM tracks;") );
    if( tracks.size() <= 0 )
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
    return StorageManager::instance()->sqlStorage()->query( sql );
}

QString
AmarokCollectionScript::escape( const QString& sql ) const
{
    return StorageManager::instance()->sqlStorage()->escape( sql );
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
