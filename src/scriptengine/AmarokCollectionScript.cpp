/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "AmarokCollectionScript.h"

#include "App.h"
#include "collection/CollectionManager.h"
#include "collection/SqlStorage.h"
#include "collection/sqlcollection/SqlCollectionLocation.h"

#include <QtScript>

namespace AmarokScript
{
    AmarokCollectionScript::AmarokCollectionScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
        Q_UNUSED( ScriptEngine );
    }

    AmarokCollectionScript::~AmarokCollectionScript()
    {
    }

    int AmarokCollectionScript::totalAlbums()
    {
        SqlStorage *s = CollectionManager::instance()->sqlStorage();
        Q_ASSERT(s);
        QStringList albums = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM albums;" );
        if( albums.size() < 1 )
            return 0;
        QString total = albums[0];
            return total.toInt();
    }

    int AmarokCollectionScript::totalArtists()
    {
        QStringList artists = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM artists;" );
        if( artists.size() < 1 )
            return 0;
        QString total = artists[0];
        return total.toInt();
    }

    int AmarokCollectionScript::totalComposers()
    {
        QStringList composers = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM composers;" );
        if( composers.size() < 1 )
            return 0;
        QString total = composers[0];
        return total.toInt();
    }

    int AmarokCollectionScript::totalGenres()
    {
        QStringList genres = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM genres;" );
        if( genres.size() < 1 )
            return 0;
        QString total = genres[0];
        return total.toInt();
    }

    int AmarokCollectionScript::totalTracks()
    {
        QStringList tracks = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( url ) FROM tracks;" );
        if( tracks.size() < 0 )
            return 0;
        QString total = tracks[0];
        int final = total.toInt();
        return final;
    }

    QStringList AmarokCollectionScript::collectionLocation()
    {
        CollectionLocation *location = CollectionManager::instance()->primaryCollection()->location();
        QStringList result = location->actualLocation();
        delete location;
        return result;
    }

    QStringList AmarokCollectionScript::query( const QString& sql )
    {
        return CollectionManager::instance()->sqlStorage()->query( sql );
    }

    void AmarokCollectionScript::scanCollection()
    {
        CollectionManager::instance()->startFullScan();
    }

    void AmarokCollectionScript::scanCollectionChanges()
    {
        CollectionManager::instance()->checkCollectionChanges();
    }

    bool AmarokCollectionScript::isDirInCollection( const QString& path )
    {
        Q_UNUSED( path );
        return false;
    }


}

#include "AmarokCollectionScript.moc"
