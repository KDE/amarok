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
#include "core-impl/collections/support/CollectionManager.h"
#include "core/collections/support/SqlStorage.h"
#include "core-impl/collections/sqlcollection/SqlCollectionLocation.h"
#include "core/support/Debug.h"


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

    int AmarokCollectionScript::totalAlbums() const
    {
        QStringList albums = query( "SELECT COUNT( id ) FROM albums;" );
        if( albums.size() < 1 )
            return 0;
        QString total = albums[0];
        return total.toInt();
    }

    int AmarokCollectionScript::totalArtists() const
    {
        QStringList artists = query( "SELECT COUNT( id ) FROM artists;" );
        if( artists.size() < 1 )
            return 0;
        QString total = artists[0];
        return total.toInt();
    }

    int AmarokCollectionScript::totalComposers() const
    {
        QStringList composers = query( "SELECT COUNT( id ) FROM composers;" );
        if( composers.size() < 1 )
            return 0;
        QString total = composers[0];
        return total.toInt();
    }

    int AmarokCollectionScript::totalGenres() const
    {
        QStringList genres = query( "SELECT COUNT( id ) FROM genres;" );
        if( genres.size() < 1 )
            return 0;
        QString total = genres[0];
        return total.toInt();
    }

    int AmarokCollectionScript::totalTracks() const
    {
        QStringList tracks = query( "SELECT COUNT( url ) FROM tracks;" );
        if( tracks.size() < 0 )
            return 0;
        QString total = tracks[0];
        int final = total.toInt();
        return final;
    }

    QStringList AmarokCollectionScript::collectionLocation() const
    {
        Collections::CollectionLocation *location = CollectionManager::instance()->primaryCollection()->location();
        QStringList result = location->actualLocation();
        delete location;
        return result;
    }

    QStringList AmarokCollectionScript::query( const QString& sql ) const
    {
        return CollectionManager::instance()->sqlStorage()->query( sql );
    }

    QString AmarokCollectionScript::escape( const QString& sql ) const
    {
        return CollectionManager::instance()->sqlStorage()->escape( sql );
    }

    void AmarokCollectionScript::scanCollection() const
    {
        CollectionManager::instance()->startFullScan();
    }

    void AmarokCollectionScript::scanCollectionChanges() const
    {
        CollectionManager::instance()->checkCollectionChanges();
    }

    bool AmarokCollectionScript::isDirInCollection( const QString& path ) const
    {
        DEBUG_BLOCK

        KUrl url = KUrl( path );
        KUrl parentUrl;
        foreach( const QString &dir, collectionLocation() )
        {
            debug() << "Collection Location: " << dir;
            debug() << "path: " << path;
            debug() << "scan Recursively: " << AmarokConfig::scanRecursively();
            parentUrl.setPath( dir );
            if ( !AmarokConfig::scanRecursively() )
            {
                if ( ( dir == path ) || ( dir + '/' == path ) )
                    return true;
            }
            else //scan recursively
            {
                if ( parentUrl.isParentOf( path ) )
                    return true;
            }
        }
        return false;
    }

    void
    AmarokCollectionScript::dumpDatabaseContent() const
    {
        //this method assumes that CollectionManager::primaryCollection() returns the
        //SqlCollection instance. It then uses Qt magic to dump the whole database to CSV files
        //in the user's home directory.
        //for debugging purposes only! do not ever use code like this for anything else
        QTimer::singleShot( 0, CollectionManager::instance()->primaryCollection(), SLOT( dumpDatabaseContent() ) );
    }
}

#include "AmarokCollectionScript.moc"

