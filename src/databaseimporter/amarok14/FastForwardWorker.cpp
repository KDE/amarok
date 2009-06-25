/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "FastForwardWorker.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "CollectionLocation.h"
#include "Debug.h"
#include "collection/support/FileCollectionLocation.h"
#include "StatisticsCapability.h"
#include "meta/file/File.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QSqlError>
#include <QSqlQuery>

FastForwardWorker::FastForwardWorker()
    : ThreadWeaver::Job()
    , m_aborted( false )
    , m_failed( false )
    , m_driver( FastForwardImporter::SQLite )
    , m_databaseLocation()
    , m_database()
    , m_hostname()
    , m_username()
    , m_password()
    , m_importArtwork( false )
{
}

QSqlDatabase
FastForwardWorker::databaseConnection()
{
    DEBUG_BLOCK

    const bool isSqlite = m_driver == FastForwardImporter::SQLite;

    QString driver = driverName();
    if( driver.isEmpty() )
    {
        emit importError( i18n("No database driver was selected") );
        m_failed = true;
        return QSqlDatabase();
    }

    if( isSqlite && !QFile::exists( m_databaseLocation ) )
    {
        emit importError( i18n("Database could not be found at: %1", m_databaseLocation ) );
        m_failed = true;
        return QSqlDatabase();
    }

    QSqlDatabase connection = QSqlDatabase::addDatabase( driver );
    connection.setDatabaseName( isSqlite ? m_databaseLocation : m_database );

    if( !isSqlite )
    {
        connection.setHostName( m_hostname );
        connection.setUserName( m_username );
        connection.setPassword( m_password );
    }
    return connection;
}

const QString
FastForwardWorker::driverName() const
{
    switch( m_driver )
    {
        case FastForwardImporter::SQLite:     return "QSQLITE";
        case FastForwardImporter::MySQL:      return "QMYSQL";
        case FastForwardImporter::PostgreSQL: return "QPSQL";
    }
    return QString();
}

void
FastForwardWorker::run()
{
    DEBUG_BLOCK

    QSqlDatabase db = databaseConnection();
    if( !db.open() )
    {
        QString errorMsg = i18n( "Could not open Amarok 1.4 database: %1", db.lastError().text() );
        debug() << errorMsg;
        emit importError( errorMsg );
        m_failed = true;
        return;
    }


    QString sql;
    sql += "SELECT lastmountpoint, S.url, createdate, accessdate, percentage, rating, playcounter, lyrics ";
    sql += "FROM statistics S ";
    sql += "LEFT OUTER JOIN devices D ";
    sql += "  ON S.deviceid = D.id ";
    sql += "LEFT OUTER JOIN lyrics L ";
    sql += "  ON L.deviceid = S.deviceid ";
    sql += "  AND L.url = S.url ";
    sql += "ORDER BY lastmountpoint, S.url";
    QSqlQuery query = db.exec( sql );

    QMap<Meta::TrackPtr,QString> tracksForInsert;

    uint i = 0;
    for( int c = 0; query.next(); c++ )
    {
        if( m_aborted )
            return;

        int index = 0;
        QString mount = query.value( index++ ).toString();
        QString url   = query.value( index++ ).toString();

        uint firstPlayed = query.value( index++ ).toUInt();
        uint lastPlayed  = query.value( index++ ).toUInt();
        uint score       = query.value( index++ ).toInt();
        int rating       = query.value( index++ ).toInt();
        int playCount    = query.value( index++ ).toInt();
        QString lyrics   = query.value( index++ ).toString();
        
        // remove the relative part of the url, and make the url absolute
        url = mount + url.mid(1);

        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
        if( track )
        {
            Meta::StatisticsCapability *ec = track->create<Meta::StatisticsCapability>();
            if( !ec )
                continue;

            ec->beginStatisticsUpdate();
            ec->setScore( score );
            ec->setRating( rating );
            ec->setFirstPlayed( firstPlayed );
            ec->setLastPlayed( lastPlayed );
            ec->setPlayCount( playCount );
            ec->endStatisticsUpdate();
            
            if( !lyrics.isEmpty() )
                track->setCachedLyrics( lyrics );

            if( !track->inCollection() )
            {
                tracksForInsert.insert( track, track->playableUrl().url() );
                debug() << c << " inserting track:" << track->playableUrl();
            }
            else
                debug() << c << " track in collection (" << track->collection()->location()->prettyLocation() << "):" << track->playableUrl();

            emit trackAdded( track );
            ++i;
        } else
            debug() << c << " no track produced for URL " << url;
    }

    if( tracksForInsert.size() > 0 )
    {
        emit showMessage( i18n( "Synchronizing Amarok database..." ) );
        CollectionLocation *location = CollectionManager::instance()->primaryCollection()->location();
        location->insertTracks( tracksForInsert );
        location->insertStatistics( tracksForInsert );
    }

    if( m_importArtwork )
    {
        const QString message = i18n( "Importing downloaded album art" );
        emit showMessage( message );

        QString newCoverPath = Amarok::saveLocation( "albumcovers/large/" );
        QDir newCoverDir( newCoverPath );
        QDir oldCoverDir( m_importArtworkDir ); 

        if( newCoverDir.canonicalPath() == oldCoverDir.canonicalPath() )
            return;

        oldCoverDir.setFilter( QDir::Files | QDir::NoDotAndDotDot );

        debug() << "new covers:" << newCoverPath;
        debug() << "old covers:" << m_importArtworkDir;

        foreach( QFileInfo image, oldCoverDir.entryInfoList() )
        {
            if( m_aborted )
                return;

            debug() << "image copy:" << image.fileName() << " : " << image.absoluteFilePath();
            QString newPath = newCoverDir.absoluteFilePath( image.fileName() );

            KUrl src( image.absoluteFilePath() );
            KUrl dst( newPath );

            //TODO: should this be asynchronous?
            KIO::FileCopyJob *job = KIO::file_copy( src, dst, -1 /*no special perms*/ , KIO::HideProgressInfo );
            if( !job->exec() ) // job deletes itself
                error() << "Couldn't copy image" << image.fileName();
        }
    }
}

