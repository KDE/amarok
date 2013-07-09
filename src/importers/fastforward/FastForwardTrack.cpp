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
#include <QVariant>

namespace StatSyncing
{

FastForwardTrack::FastForwardTrack( const QString &trackUrl, const QString &providerUid )
    : m_trackUrl( trackUrl )
    , m_providerUid( providerUid )
{
    QSqlDatabase db = QSqlDatabase::database( m_providerUid );
    if( !db.isOpen() )
    {
        warning() << __PRETTY_FUNCTION__ << "could not open database connection:"
                     << db.lastError().text();
        return;
    }

    QSqlQuery query( db );
    query.setForwardOnly( true ); // a hint for the database engine

    query.prepare( "SELECT t.title, al.name, ar.name, c.name, y.name, t.track,"
                   "t.discnumber FROM tags t"
                   "LEFT JOIN artist ar ON ar.id = t.artist"
                   "LEFT JOIN album al ON al.id = t.album"
                   "LEFT JOIN composer c ON c.id = t.composer"
                   "LEFT JOIN year y ON y.id = t.year"
                   "WHERE t.url = ?" );
    query.addBindValue( m_trackUrl );
    query.exec();

    if( query.next() )
    {
        QList<quint64> fields;
        fields << Meta::valTitle << Meta::valAlbum << Meta::valArtist << Meta::valComposer
               << Meta::valYear << Meta::valTrackNr << Meta::valDiscNr;

        for( int i = 0; i < fields.size(); ++i )
            m_metadata[fields[i]] = query.value( i );
    }
}

FastForwardTrack::~FastForwardTrack()
{
}

QString
FastForwardTrack::name() const
{
    return m_metadata[Meta::valTitle].toString();
}

QString
FastForwardTrack::album() const
{
    return m_metadata[Meta::valAlbum].toString();
}

QString
FastForwardTrack::artist() const
{
    return m_metadata[Meta::valArtist].toString();
}

QString
FastForwardTrack::composer() const
{
    return m_metadata[Meta::valComposer].toString();
}

int
FastForwardTrack::year() const
{
    return m_metadata[Meta::valYear].toInt();
}

int
FastForwardTrack::trackNumber() const
{
    return m_metadata[Meta::valTrackNr].toInt();
}

int
FastForwardTrack::discNumber() const
{
    return m_metadata[Meta::valDiscNr].toInt();
}

int
FastForwardTrack::rating() const
{
    checkAllDataRetrieved();
    return m_statistics[Meta::valRating].toInt();
}

QDateTime
FastForwardTrack::firstPlayed() const
{
    checkAllDataRetrieved();
    const QVariant &t = m_statistics[Meta::valFirstPlayed];
    return t.isNull() ? QDateTime() : QDateTime::fromTime_t( t.toUInt() );
}

QDateTime
FastForwardTrack::lastPlayed() const
{
    checkAllDataRetrieved();
    const QVariant &t = m_statistics[Meta::valLastPlayed];
    return t.isNull() ? QDateTime() : QDateTime::fromTime_t( t.toUInt() );
}

int
FastForwardTrack::playCount() const
{
    checkAllDataRetrieved();
    return m_statistics[Meta::valPlaycount].toInt();
}

QSet<QString>
FastForwardTrack::labels() const
{
    checkAllDataRetrieved();
    return m_labels;
}

void
FastForwardTrack::checkAllDataRetrieved() const
{
    QMutexLocker lock( &m_statMutex );
    if( !m_statistics.empty() )
        return;

    // SQL queries need to be executed in the main thread, and we can't use
    // BlockingQueuedConnection if we're already in the main thread
    const Qt::ConnectionType connectionType =
            this->thread() == QCoreApplication::instance()->thread()
            ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

    QMetaObject::invokeMethod( const_cast<FastForwardTrack*>( this ),
                               "retrievePersonalData", connectionType );
}

void
FastForwardTrack::retrievePersonalData()
{
    QSqlDatabase db = QSqlDatabase::database( m_providerUid );
    if( !db.isOpen() )
    {
        warning() << __PRETTY_FUNCTION__ << "could not open database connection:"
                     << db.lastError().text();
        return;
    }

    QSqlQuery query( db );
    query.setForwardOnly( true ); // a hint for the database engine

    query.prepare( "SELECT rating, createdate, accessdate, playcounter"
                   "FROM statistics WHERE url = ?" );
    query.addBindValue( m_trackUrl );
    query.exec();

    if( query.next() )
    {
        QList<quint64> fields;
        fields << Meta::valRating << Meta::valFirstPlayed << Meta::valLastPlayed
                  << Meta::valPlaycount;

        for( int i = 0; i < fields.size(); ++i )
            m_statistics[fields[i]] = query.value( i );
    }

    query.prepare( "SELECT l.name FROM labels l"
                   "INNER JOIN tags_labels tl ON tl.labelid = l.id"
                   "WHERE tl.url = ?");
    query.addBindValue( m_trackUrl );
    query.exec();

    while( query.next() )
        m_labels.insert( query.value( 0 ).toString() );
}

} // namespace StatSyncing
