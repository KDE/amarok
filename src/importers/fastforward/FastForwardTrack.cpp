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

#include <QCoreApplication>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

using namespace StatSyncing;

FastForwardTrack::FastForwardTrack( const Meta::FieldHash &metadata,
                                    const QString &trackUrl, const QString &providerUid )
    : SimpleTrack( metadata )
    , m_providerUid( providerUid )
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
    assureStatisticsRetrieved();
    const uint t = m_statistics.value( Meta::valFirstPlayed ).toUInt();
    return t == 0 ? QDateTime() : QDateTime::fromTime_t( t );
}

QDateTime
FastForwardTrack::lastPlayed() const
{
    assureStatisticsRetrieved();
    const uint t = m_statistics.value( Meta::valLastPlayed ).toUInt();
    return t == 0 ? QDateTime() : QDateTime::fromTime_t( t );
}

int
FastForwardTrack::rating() const
{
    assureStatisticsRetrieved();
    return m_statistics.value( Meta::valRating ).toInt();
}

int
FastForwardTrack::playCount() const
{
    assureStatisticsRetrieved();
    return m_statistics.value( Meta::valPlaycount ).toInt();
}

QSet<QString>
FastForwardTrack::labels() const
{
    assureStatisticsRetrieved();
    return m_labels;
}

void
FastForwardTrack::assureStatisticsRetrieved() const
{
    QMutexLocker lock( &m_mutex );
    if( m_statisticsRetrieved )
        return;

    const Qt::ConnectionType connectionType =
            this->thread() == QCoreApplication::instance()->thread()
            ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

    QMetaObject::invokeMethod( const_cast<FastForwardTrack*>( this ),
                               "retrieveStatistics", connectionType );
}

void
FastForwardTrack::retrieveStatistics()
{
    QSqlDatabase db = QSqlDatabase::database( m_providerUid );
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
