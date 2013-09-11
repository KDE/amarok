/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "FastForwardTrack.h"

#include "core/support/Debug.h"

#include <ThreadWeaver/Thread>

#include <QWriteLocker>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

using namespace StatSyncing;

FastForwardTrack::FastForwardTrack( const ImporterSqlProviderPtr &provider,
                                    const Meta::FieldHash &metadata,
                                    const QString &trackUrl )
    : ImporterSqlTrack( provider, metadata )
    , m_statisticsRetrieved( false )
    , m_trackUrl( trackUrl )
{
}

FastForwardTrack::~FastForwardTrack()
{
}

QDateTime
FastForwardTrack::firstPlayed() const
{
    QWriteLocker lock( &m_lock );
    assureStatisticsRetrieved();
    return getDateTime( m_statistics.value( Meta::valFirstPlayed ) );
}

QDateTime
FastForwardTrack::lastPlayed() const
{
    QWriteLocker lock( &m_lock );
    assureStatisticsRetrieved();
    return getDateTime( m_statistics.value( Meta::valLastPlayed ) );
}

int
FastForwardTrack::rating() const
{
    QWriteLocker lock( &m_lock );
    assureStatisticsRetrieved();
    return m_statistics.value( Meta::valRating ).toInt();
}

int
FastForwardTrack::playCount() const
{
    QWriteLocker lock( &m_lock );
    assureStatisticsRetrieved();
    return m_statistics.value( Meta::valPlaycount ).toInt();
}

QSet<QString>
FastForwardTrack::labels() const
{
    QWriteLocker lock( &m_lock );
    assureStatisticsRetrieved();
    return m_labels;
}

void
FastForwardTrack::assureStatisticsRetrieved() const
{
    if( !m_statisticsRetrieved )
        QMetaObject::invokeMethod( const_cast<FastForwardTrack*>( this ),
                                   "retrieveStatistics", blockingConnectionType() );
}

void
FastForwardTrack::retrieveStatistics()
{
    prepareConnection();
    QSqlDatabase db = QSqlDatabase::database( connectionName() );

    QSqlQuery query( db );
    query.setForwardOnly( true );

    query.prepare( "SELECT rating, createdate, accessdate, playcounter "
                   "FROM statistics "
                   "WHERE url = ?" );
    query.addBindValue( m_trackUrl );
    query.exec();

    if( query.next() )
    {
        const QList<qint64> fields = QList<qint64>() << Meta::valRating
                    << Meta::valFirstPlayed << Meta::valLastPlayed << Meta::valPlaycount;

        for( int i = 0; i < fields.size(); ++i )
            m_statistics.insert( fields[i], query.value( i ) );
    }
    else
    {
        warning() << __PRETTY_FUNCTION__ << "could not retrieve personal track metadata."
                  << "track url:" << m_trackUrl << "error:" << query.lastError().text();
    }

    query.prepare( "SELECT l.name FROM labels l "
                   "INNER JOIN tags_labels tl ON tl.labelid = l.id "
                   "WHERE tl.url = ?");
    query.addBindValue( m_trackUrl );
    query.exec();

    while( query.next() )
        m_labels.insert( query.value( 0 ).toString() );

    m_statisticsRetrieved = true;
}

void
StatSyncing::FastForwardTrack::sqlCommit( QSqlDatabase db, const QSet<qint64> &fields )
{
    db.transaction();
    QSqlQuery query( db );

    query.prepare( "SELECT deviceid, uniqueid FROM uniqueid WHERE url = ?" );
    query.addBindValue( m_trackUrl );
    query.exec();

    if( !query.next() )
    {
        db.rollback();
        return;
    }

    const int deviceId = query.value( 0 ).toInt();
    const QString uniqueId = query.value( 1 ).toString();

    QStringList updates;
    if( fields.contains( Meta::valFirstPlayed ) )
        updates << "createdate = :createdate";
    if( fields.contains( Meta::valLastPlayed ) )
        updates << "accessdate = :accessdate";
    if( fields.contains( Meta::valRating ) )
        updates << "rating = :rating";
    if( fields.contains( Meta::valPlaycount ) )
        updates << "playcounter = :playcount";

    if( !updates.isEmpty() )
    {
        query.prepare( "SELECT COUNT(*) FROM statistics WHERE url = ?" );
        query.addBindValue( m_trackUrl );
        query.exec();

        if( !query.next() )
        {
            db.rollback();
            return;
        }

        // Statistic row doesn't exist
        if( !query.value( 0 ).toInt() )
        {
            query.prepare( "INSERT INTO statistics (url, deviceid, uniqueid) "
                           "VALUES (?, ?, ?)" );
            query.addBindValue( m_trackUrl );
            query.addBindValue( deviceId );
            query.addBindValue( uniqueId );

            if( !query.exec() )
            {
                db.rollback();
                return;
            }
        }

        // Update statistics
        query.prepare( "UPDATE statistics SET "+updates.join(", ")+" WHERE url = :url" );
        query.bindValue( ":createdate", m_statistics.value( Meta::valFirstPlayed )
                                                               .toDateTime().toTime_t() );
        query.bindValue( ":accessdate", m_statistics.value( Meta::valLastPlayed )
                                                               .toDateTime().toTime_t() );
        query.bindValue( ":rating", m_statistics.value( Meta::valRating ).toInt() );
        query.bindValue( ":playcount", m_statistics.value( Meta::valPlaycount ).toInt() );
        query.bindValue( ":url", m_trackUrl );

        if( !query.exec() )
        {
            db.rollback();
            return;
        }
    }

    if( fields.contains( Meta::valLabel ) )
    {
        foreach( const QString &label, m_labels )
        {
            // Check if a label exists
            query.prepare( "SELECT COUNT(*) FROM labels WHERE name = ?" );
            query.addBindValue( label );
            query.exec();

            if( !query.next() )
            {
                db.rollback();
                return;
            }

            // Insert label
            if( !query.value( 0 ).toInt() )
            {
                query.prepare( "INSERT INTO labels (name, type) VALUES (?, 1)" );
                query.addBindValue( label );

                if( !query.exec() )
                {
                    db.rollback();
                    return;
                }
            }

            // We can't use lastInsertId because we can't be sure if driver supports it,
            // so to simplify logic we use check if exists -> insert if not -> select
            query.prepare( "SELECT id FROM labels WHERE name = ?" );
            query.addBindValue( label );
            query.exec();

            if( !query.next() )
            {
                db.rollback();
                return;
            }

            const qint64 labelId = query.value( 0 ).toLongLong();

            query.prepare( "SELECT COUNT(*) FROM tags_labels WHERE url=? AND labelid=?" );
            query.addBindValue( m_trackUrl );
            query.addBindValue( labelId );
            query.exec();

            if( !query.next() )
            {
                db.rollback();
                return;
            }

            // Insert track <-> label connection
            if( !query.value( 0 ).toInt() )
            {
                query.prepare( "INSERT INTO tags_labels (deviceid, url, uniqueid, "
                               "labelid) VALUES (?, ?, ?, ?)" );
                query.addBindValue( deviceId );
                query.addBindValue( m_trackUrl );
                query.addBindValue( uniqueId );
                query.addBindValue( labelId );

                if( !query.exec() )
                {
                    db.rollback();
                    return;
                }
            }
        }
    }

    db.commit();
}
