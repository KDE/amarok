/***************************************************************************
 *   Copyright (C) 2007 by                                                 *
 *      Last.fm Ltd <client@last.fm>                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "Collection.h"
#include "lib/core/CoreDir.h"

#include <QStringList>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

static const int k_collectionDbVersion = 1;

// Singleton instance needs to be initialised
Collection* Collection::s_instance = NULL;


Collection::Collection()
{
    initDatabase();
}


Collection::~Collection()
{
    QSqlDatabase::removeDatabase( "collection" );
    m_db.close();
}


Collection&
Collection::instance()
{
    static QMutex mutex;
    QMutexLocker locker( &mutex );

    if ( !s_instance )
    {
        s_instance = new Collection;
    }

    return *s_instance;
}


bool
Collection::initDatabase()
{
    QMutexLocker locker_q( &m_mutex );

    if ( !m_db.isValid() )
    {
        m_db = QSqlDatabase::addDatabase( "QSQLITE", "collection" );

        if ( m_dbPath.isEmpty() )
            m_db.setDatabaseName( CoreDir::data().filePath( "/collection.db" ) );
        else
            m_db.setDatabaseName( m_dbPath );
    }
    
    if( !m_db.open())
    {
        qDebug() << "Could not open sqlite database: " << m_db.databaseName() << ". Error: " << m_db.lastError();
        return false;
    }

    qDebug() << "Opening Collection database" << ( m_db.isValid() ? "worked" : "failed" );
    if ( !m_db.isValid() )
        return false;

    if ( !m_db.tables().contains( "files" ) )
    {
        qDebug() << "Creating Collection database!";

        query( "CREATE TABLE artists ("
                    "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "serverUid   INTEGER,"
                    "lcName      TEXT NOT NULL,"
                    "displayName TEXT NOT NULL );" );

        query( "CREATE TABLE albums ("
                    "id            INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "serverUid     INTEGER,"
                    "lcName        TEXT NOT NULL,"
                    "displayName   TEXT NOT NULL,"
                    "primaryArtist INTEGER NOT NULL );" );

        query( "CREATE UNIQUE INDEX album_names_idx ON albums ( primaryArtist, lcName );" );

        query( "CREATE TABLE tracks ("
                    "id                INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "lcName            TEXT NOT NULL,"
                    "displayName       TEXT NOT NULL,"
                    "primaryArtist     INTEGER NOT NULL,"
                    "primaryAlbum      INTEGER );" );

        query( "CREATE UNIQUE INDEX track_names_idx ON tracks ( primaryArtist, lcName );" );

        query( "CREATE TABLE files ("
                    "id                INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "uri               TEXT NOT NULL,"
                    "track             INTEGER NOT NULL,"
                    "bitrate           INTEGER,"
                    "samplerate        INTEGER,"
                    "duration          INTEGER,"
                    "filesize          INTEGER,"
                    "source            INTEGER,"
                    "modificationDate  INTEGER,"
                    "lastPlayDate      INTEGER,"
                    "playCounter       INTEGER,"
                    "mbId              VARCHAR( 36 ),"
                    "fpId              INTEGER );" );

        query( "CREATE UNIQUE INDEX files_uri_idx ON files ( uri );" );
        query( "CREATE INDEX files_track_idx ON files ( track );" );
        query( "CREATE INDEX files_fpId_idx ON files ( fpId );" );
        query( "CREATE INDEX files_source_idx ON files ( source );" );

        query( "CREATE TABLE sources ("
                    "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "name        TEXT UNIQUE,"
                    "available   INTEGER,"
                    "host        TEXT,"
                    "cost        INTEGER );" );

        query( "CREATE TABLE genres ("
                    "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "name        TEXT UNIQUE );" );

        query( "CREATE TABLE labels ("
                    "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "serverUid   INTEGER UNIQUE,"
                    "name        TEXT );" );
    }
    
    int const v = version();
    if ( v < k_collectionDbVersion )
    {
        qDebug() << "Upgrading Collection::db from" << v << "to" << k_collectionDbVersion;

        /**********************************************
         * README!!!!!!!                              *
         * Ensure you use v < x                       *
         * Ensure you do upgrades in ascending order! *
         **********************************************/
    
        if ( v < 1 )
        {
            // Norman discovered that he stored some fpId's wrong prior to 17th December 2007
            // So we have to wipe the fpIds for databases without the metadata table
            // we didn't store version information before that, which was a bad decision wasn't it?

            // this will trigger refingerprinting of every track
            query( "UPDATE files SET fpId = NULL;" );

            query( "CREATE TABLE metadata ("
                        "key         TEXT UNIQUE NOT NULL,"
                        "value       TEXT );" );

            query( "INSERT INTO metadata (key, value) VALUES ('version', '1');" );
        }


        // do last, update DB version number
        query( "UPDATE metadata set key='version', value='"
                    + QString::number( k_collectionDbVersion ) + "';" );
    }

    return true;
}


int
Collection::version() const 
{   
    QSqlQuery sql( m_db );
    sql.exec( "SELECT value FROM metadata WHERE key='version';" );

    if ( sql.next() )
    {
        return sql.value( 0 ).toInt();
    }

    return 0;
}

bool
Collection::query( const QString& queryToken )
{
    QSqlQuery query( m_db );
    query.exec( queryToken );

    if ( query.lastError().isValid() )
    {
        qDebug() << "SQL query failed:" << query.lastQuery() << endl
                 << "SQL error was:"    << query.lastError().databaseText() << endl
                 << "SQL error type:"   << query.lastError().type();

        return false;
    }

    return true;
}


QString
Collection::fileURI( const QString& filePath )
{
    QString prefix( "file:/" );

#ifdef WIN32
    prefix = "file://";
#endif

    return prefix + QFileInfo( filePath ).absoluteFilePath();
}


QString
Collection::getFingerprint( const QString& filePath )
{
    QSqlQuery query( m_db );
    query.prepare( "SELECT fpId FROM files WHERE uri = :uri" );
    query.bindValue( ":uri", fileURI( filePath ) );

    query.exec();
    if ( query.lastError().isValid() )
    {
        qDebug() << "SQL query failed:" << query.lastQuery() << endl
                 << "SQL error was:"    << query.lastError().databaseText() << endl
                 << "SQL error type:"   << query.lastError().type();
    }
    else if ( query.next() )
        return query.value( 0 ).toString();

    return "";
}


bool
Collection::setFingerprint( const QString& filePath, QString fpId )
{
    bool isNumeric;
    int intFpId = fpId.toInt( &isNumeric );
    Q_ASSERT( isNumeric );

    QSqlQuery query( m_db );
    query.prepare( "REPLACE INTO files ( uri, track, fpId ) VALUES ( :uri, 0, :fpId )" );
    query.bindValue( ":uri", fileURI( filePath ) );
    query.bindValue( ":fpId", intFpId );
    query.exec();

    if ( query.lastError().isValid() )
    {
        qDebug() << "SQL query failed:" << query.lastQuery() << endl
                 << "SQL error was:"    << query.lastError().databaseText() << endl
                 << "SQL error type:"   << query.lastError().type();

        return false;
    }

    return true;
}


void
Collection::destroy()
{
    delete s_instance;
}
