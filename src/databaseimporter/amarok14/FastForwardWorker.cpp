/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "FastForwardWorker.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "CollectionLocation.h"
#include "Debug.h"
#include "ImportCapability.h"
#include "meta/file/File.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

FastForwardWorker::FastForwardWorker()
    : ThreadWeaver::Job()
    , m_aborted( false )
    , m_failed( false )
    , m_importArtwork( false )
{
}

QSqlDatabase
FastForwardWorker::databaseConnection()
{
    DEBUG_BLOCK

    const bool isSqlite = m_driver == FastForwardImporter::SQLite;

    QSqlDatabase connection = QSqlDatabase::addDatabase( driverName() );
    connection.setDatabaseName( isSqlite ? m_databaseLocation : m_database );

    if( m_driver == FastForwardImporter::MySQL || m_driver == FastForwardImporter::PostgreSQL )
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
    sql += "INNER JOIN devices D ";
    sql += "  ON S.deviceid = D.id ";
    sql += "LEFT OUTER JOIN lyrics L ";
    sql += "  ON L.deviceid = S.deviceid ";
    sql += "  AND L.url = S.url ";
    sql += "ORDER BY lastmountpoint, S.url";
    QSqlQuery query = db.exec( sql );

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
        
        // make the url absolute
        url = mount + url.mid(1);

        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
        if( track )
        {
            if( !track->inCollection() )
            {
                 // It's a Meta::FileTrack hopefully
                KSharedPtr<MetaFile::Track> fileTrack = KSharedPtr<MetaFile::Track>::dynamicCast( track );
                if( !fileTrack )
                    continue;

                debug() << c << "  inserting track:" << fileTrack->playableUrl();
            }
            else
            {
                debug() << c << "  track already in collection (" << track->collection()->location()->prettyLocation() << "):" << track->playableUrl();

                Meta::ImportCapability *ec = track->as<Meta::ImportCapability>();
                if( !ec )
                    continue;

                ec->beginStatisticsUpdate();
                ec->setScore( score );
                ec->setRating( rating );
                ec->setFirstPlayed( firstPlayed );
                ec->setLastPlayed( lastPlayed );
                ec->setPlayCount( playCount );
                ec->endStatisticsUpdate();

                debug() << c << "   --> updating track:" << track->playableUrl();

                if( !lyrics.isEmpty() )
                    track->setCachedLyrics( lyrics );
            }
            
            emit trackAdded( track );
        }
    }

    if( m_importArtwork )
    {
        QString message = i18n( "Importing downloaded album art" );
        emit sendMessage( message );

        // FIXME: determining the old cover art directory is a major hack, I admit.
        // What's the best way of doing this?
        QString newCoverPath = Amarok::saveLocation( "albumcovers/large/" );
        QString oldCoverPath = QString( newCoverPath );
        oldCoverPath = oldCoverPath.replace( ".kde4", ".kde" );
        QDir newCoverDir( newCoverPath );
        QDir oldCoverDir( oldCoverPath ); 
        oldCoverDir.setFilter( QDir::Files | QDir::NoDotAndDotDot );

        debug() << "new covers:" << newCoverPath;
        debug() << "old covers:" << oldCoverPath;

        foreach( QFileInfo image, oldCoverDir.entryInfoList() )
        {
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

