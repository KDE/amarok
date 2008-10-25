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
#include "CollectionManager.h"
#include "Debug.h"
#include "ImportCapability.h"

#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

FastForwardWorker::FastForwardWorker()
    : ThreadWeaver::Job()
    , m_aborted( false )
    , m_failed( false )
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
    sql += "SELECT lastmountpoint, url, createdate, accessdate, percentage, rating, playcounter FROM statistics S, devices D where S.deviceid = D.id ";
    sql += "ORDER BY lastmountpoint, url";
    QSqlQuery query = db.exec( sql );

    for( int c = 0; query.next(); c++ )
    {
        int index = 0;
        QString mount = query.value( index++ ).toString();
        QString url   = query.value( index++ ).toString();

        uint firstPlayed = query.value( index++ ).toUInt();
        uint lastPlayed  = query.value( index++ ).toUInt();
        uint score       = query.value( index++ ).toInt();
        int rating      = query.value( index++ ).toInt();
        int playCount   = query.value( index++ ).toInt();
        
        // make the url absolute
        url = mount + url.mid(1);

        // Check whether we already have the track in a build collection
        //SqlMeta::TrackPtr track = SqlMeta::TrackPtr::dynamicCast( CollectionManager::instance()->trackForUrl( KUrl( url ) ) );
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );
        if( track )
        {
            debug() << c << "  retrieved track:" << track->playableUrl();
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

            //TODO: import lyrics
        }
        // We need to create a new track and have it inserted, if the URL exists
        else
        {
            QFileInfo info( url );
            if( !info.exists() )
            {
                // Bail out if we can't find the track on the filesystem
                debug() << c << "  couldn't find:" << url;
                continue;
            }
            debug() << c << "  inserting track:" << url;
            //TODO complete
        }

        if( track )
            emit trackAdded( track );
    }

    // TODO: copy downloaded artwor
}

