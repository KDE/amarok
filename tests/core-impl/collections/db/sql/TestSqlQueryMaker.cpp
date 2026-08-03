/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestSqlQueryMaker.h"

#include "amarokconfig.h"
#include "core-impl/storage/sql/mysqlestorage/MySqlEmbeddedStorage.h"
#include "../SqlMountPointManagerMock.h"


QTEST_GUILESS_MAIN( TestSqlQueryMaker )

void
TestSqlQueryMaker::initTestCase()
{
    if( !s_tmpDir )
        s_tmpDir = new QTemporaryDir();
    MySqlEmbeddedStorage *storage = new MySqlEmbeddedStorage();
    m_storage = QSharedPointer<MySqlEmbeddedStorage>( storage );
    QVERIFY( storage->init( s_tmpDir->path() ) );
    m_collection = new Collections::SqlCollection( m_storage );

    QMap<int,QString> mountPoints;
    mountPoints.insert( 1, QStringLiteral("/foo") );
    mountPoints.insert( 2, QStringLiteral("/bar") );

    m_mpm = new SqlMountPointManagerMock( this, m_storage );
    m_mpm->m_mountPoints = mountPoints;

    m_collection->setMountPointManager( m_mpm );

    //setup test data
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (1, 'artist1');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (2, 'artist2');") );
    m_storage->query( QStringLiteral("INSERT INTO artists(id, name) VALUES (3, 'artist3');") );

    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(1,'album1',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(2,'album2',1);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(3,'album3',2);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(4,'album4',NULL);") );
    m_storage->query( QStringLiteral("INSERT INTO albums(id,name,artist) VALUES(5,'album4',3);") );

    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (1, 'composer1');") );
    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (2, 'composer2');") );
    m_storage->query( QStringLiteral("INSERT INTO composers(id, name) VALUES (3, 'composer3');") );

    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (1, 'genre1');") );
    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (2, 'genre2');") );
    m_storage->query( QStringLiteral("INSERT INTO genres(id, name) VALUES (3, 'genre3');") );

    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (1, '1');") );
    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (2, '2');") );
    m_storage->query( QStringLiteral("INSERT INTO years(id, name) VALUES (3, '3');") );

    m_storage->query( QStringLiteral("INSERT INTO directories(id, deviceid, dir) VALUES (1, -1, './');") );

    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (1, -1, './IDoNotExist.mp3', 1, '1');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (2, -1, './IDoNotExistAsWell.mp3', 1, '2');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (3, -1, './MeNeither.mp3', 1, '3');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (4, 2, './NothingHere.mp3', 1, '4');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (5, 1, './GuessWhat.mp3', 1, '5');") );
    m_storage->query( QStringLiteral("INSERT INTO urls(id, deviceid, rpath, directory, uniqueid) VALUES (6, 2, './LookItsA.flac', 1, '6');") );

    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(1,1,'track1','comment1',1,1,1,1,1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(2,2,'track2','comment2',1,2,1,1,1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(3,3,'track3','comment3',3,4,1,1,1);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(4,4,'track4','comment4',2,3,3,3,3);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(5,5,'track5','',3,5,2,2,2);") );
    m_storage->query( QStringLiteral("INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(6,6,'track6','',1,4,2,2,2);") );

    m_storage->query( QStringLiteral("INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(1,1000,10000, 50.0,2,100);") );
    m_storage->query( QStringLiteral("INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(2,2000,30000, 70.0,9,50);") );
    m_storage->query( QStringLiteral("INSERT INTO statistics(url,createdate,accessdate,score,rating,playcount) "
                      "VALUES(3,4000,20000, 60.0,4,10);") );

    m_storage->query( QStringLiteral("INSERT INTO labels(id,label) VALUES (1,'labelA'), (2,'labelB'),(3,'test');") );
    m_storage->query( QStringLiteral("INSERT INTO urls_labels(url,label) VALUES (1,1),(1,2),(2,2),(3,3),(4,3),(4,2);") );

}
