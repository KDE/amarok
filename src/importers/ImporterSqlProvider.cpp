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

#include "ImporterSqlProvider.h"

#include <QApplication>
#include <QUuid>

using namespace StatSyncing;

ImporterSqlProvider::ImporterSqlProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
    , m_connectionName( QUuid::createUuid().toString() )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( m_config.value( "dbDriver" ).toString(),
                                                 m_connectionName );

    db.setDatabaseName( m_config.value( "dbName" ).toString() );
    db.setHostName    ( m_config.value( "dbHost" ).toString() );
    db.setUserName    ( m_config.value( "dbUser" ).toString() );
    db.setPassword    ( m_config.value( "dbPass" ).toString() );
    db.setPort        ( m_config.value( "dbPort" ).toInt()    );
}

ImporterSqlProvider::~ImporterSqlProvider()
{
    QSqlDatabase::removeDatabase( m_connectionName );
}

QSet<QString>
ImporterSqlProvider::artists()
{
    QMetaObject::invokeMethod( this, "artistsSearch", getConnectionType() );

    QSet<QString> artistSet;
    artistSet.swap( m_artistsResult );

    return artistSet;
}

TrackList
ImporterSqlProvider::artistTracks( const QString &artistName )
{
    QMetaObject::invokeMethod( this, "artistTracksSearch", getConnectionType(),
                               Q_ARG( QString, artistName ) );

    TrackList artistTrackList;
    artistTrackList.swap( m_artistTracksResult );

    return artistTrackList;
}

Qt::ConnectionType
ImporterSqlProvider::getConnectionType() const
{
    // SQL queries need to be executed in the main thread
    return this->thread() == QCoreApplication::instance()->thread()
            ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
}

void
ImporterSqlProvider::artistsSearch()
{
    Q_ASSERT( this->thread() == QCoreApplication::instance()->thread() );

    QSqlDatabase db = QSqlDatabase::database( m_connectionName );
    if( !db.isOpen() )
        return;

    m_artistsResult = getArtists( db );
}

void
ImporterSqlProvider::artistTracksSearch( const QString &artistName )
{
    Q_ASSERT( this->thread() == QCoreApplication::instance()->thread() );

    QSqlDatabase db = QSqlDatabase::database( m_connectionName );
    if( !db.isOpen() )
        return;

    m_artistTracksResult = getArtistTracks( artistName, db );
}
