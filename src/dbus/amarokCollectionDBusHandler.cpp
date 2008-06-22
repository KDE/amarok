/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (C) 2006 2007 Leonardo Franchi <lfranchi@gmail.com>              *
 *           (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
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


#include "amarokCollectionDBusHandler.h"

#include "App.h"
#include "collection/CollectionManager.h"
#include "collection/SqlStorage.h"
#include "collection/sqlcollection/SqlCollection.h"
#include "collection/sqlcollection/SqlCollectionLocation.h"
#include "Debug.h"

#include "amarokCollectionAdaptor.h"

namespace Amarok
{

    amarokCollectionDBusHandler::amarokCollectionDBusHandler()
    : QObject( kapp )
    {
        new amarokCollectionAdaptor(this);
        QDBusConnection::sessionBus().registerObject("/Collection", this);
    }

    int amarokCollectionDBusHandler::totalAlbums()
    {
        SqlStorage *s = CollectionManager::instance()->sqlStorage();
        Q_ASSERT(s);
        QStringList albums = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM albums;" );
        if( albums.size() < 1 )
            return 0;
        QString total = albums[0];
        return total.toInt();
    }

    int amarokCollectionDBusHandler::totalArtists()
    {
        QStringList artists = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM artists;" );
        if( artists.size() < 1 )
            return 0;
        QString total = artists[0];
        return total.toInt();
    }

    int amarokCollectionDBusHandler::totalComposers()
    {
        QStringList composers = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM composers;" );
        if( composers.size() < 1 )
            return 0;
        QString total = composers[0];
        return total.toInt();
    }

    int amarokCollectionDBusHandler::totalCompilations()
    {
        AMAROK_NOTIMPLEMENTED
        //Todo port 2.0
        //QStringList comps = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( DISTINCT album ) FROM tags WHERE sampler = 1;" );
        //QString total = comps[0];
        //return total.toInt();
        return -1;
    }

    int amarokCollectionDBusHandler::totalGenres()
    {
        //This should really work across multiple collections, but theres no interface for it currently..
        QStringList genres = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( id ) FROM genres;" );
        if( genres.size() < 1 )
            return 0;
        QString total = genres[0];
        return total.toInt();
    }

    int amarokCollectionDBusHandler::totalTracks()
    {
        //This should really work across multiple collections, but theres no interface for it currently..
        QStringList tracks = CollectionManager::instance()->sqlStorage()->query( "SELECT COUNT( url ) FROM tracks;" );
        if( tracks.size() < 0 )
            return 0;
        QString total = tracks[0];
        int final = total.toInt();
        return final;
    }

    bool amarokCollectionDBusHandler::isDirInCollection( const QString& path )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( path );
        return false;
    }

    QStringList amarokCollectionDBusHandler::collectionLocation()
    {
        CollectionLocation *location = CollectionManager::instance()->primaryCollection()->location();
        QStringList result = location->actualLocation();
        delete location;
        return result;
    }

    bool amarokCollectionDBusHandler::moveFile( const QString &oldURL, const QString &newURL, bool overwrite )
    {
        Q_UNUSED( oldURL ); Q_UNUSED( newURL ); Q_UNUSED( overwrite );
        AMAROK_NOTIMPLEMENTED
        return false;
        //         return CollectionDB::instance()->moveFile( oldURL, newURL, overwrite );
    }

    QStringList amarokCollectionDBusHandler::query( const QString& sql )
    {
        return CollectionManager::instance()->sqlStorage()->query( sql );
    }

    QStringList amarokCollectionDBusHandler::similarArtists( int artists )
    {
        AMAROK_NOTIMPLEMENTED
        Q_UNUSED( artists );
        //TODO: implement
        return QStringList();
    }

    void amarokCollectionDBusHandler::migrateFile( const QString &oldURL, const QString &newURL )
    {
        Q_UNUSED( oldURL ); Q_UNUSED( newURL );
        AMAROK_NOTIMPLEMENTED
        return;
        //         CollectionDB::instance()->migrateFile( oldURL, newURL );
    }

    void amarokCollectionDBusHandler::scanCollection()
    {
        CollectionManager::instance()->startFullScan();
    }

    void amarokCollectionDBusHandler::scanCollectionChanges()
    {
        CollectionManager::instance()->checkCollectionChanges();
    }

    int amarokCollectionDBusHandler::addLabels( const QString &url, const QStringList &labels )
    {
        Q_UNUSED( url ); Q_UNUSED( labels );
        AMAROK_NOTIMPLEMENTED
        return -1;
        //         CollectionDB *db = CollectionDB::instance();
        //         QString uid = db->getUniqueId( url );
        //         int count = 0;
        //         oldForeach( labels )
        //         {
        //             if( db->addLabel( url, *it, uid , CollectionDB::typeUser ) )
        //                 count++;
        //         }
        //         return count;
    }

    void amarokCollectionDBusHandler::removeLabels( const QString &url, const QStringList &oldLabels )
    {
        Q_UNUSED( url ); Q_UNUSED( oldLabels );
        AMAROK_NOTIMPLEMENTED
        return;
    }

    void amarokCollectionDBusHandler::disableAutoScoring( bool disable )
    {
        Q_UNUSED( disable );
        AMAROK_NOTIMPLEMENTED
        return;
    }
}

#include "amarokCollectionDBusHandler.moc"
