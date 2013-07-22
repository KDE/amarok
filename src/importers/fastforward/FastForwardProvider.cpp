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

#include "FastForwardProvider.h"

#include "core/support/Debug.h"
#include "FastForwardTrack.h"
#include "importers/ImporterManager.h"
#include "MetaValues.h"

#include <KLocalizedString>

#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

using namespace StatSyncing;

FastForwardProvider::FastForwardProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( m_config["dbDriver"].toString(),
            m_config["uid"].toString() );

    db.setDatabaseName( m_config["dbDriver"].toString() == "QSQLITE"
            ? m_config["dbPath"].toString() : m_config["dbName"].toString() );

    if( m_config["dbDriver"].toString() != "QSQLITE" )
    {
        db.setHostName( m_config["dbHost"].toString() );
        db.setUserName( m_config["dbUser"].toString() );
        db.setPassword( m_config["dbPass"].toString() );
        db.setPort    ( m_config["dbPort"].toInt() );
    }
}

FastForwardProvider::~FastForwardProvider()
{
    QSqlDatabase::removeDatabase( m_config["uid"].toString() );
}

qint64
FastForwardProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum | Meta::valComposer
            | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
}

qint64
FastForwardProvider::writableTrackStatsData() const
{
    //TODO: Write capabilities
    return 0;
}

QSet<QString>
FastForwardProvider::artists()
{
    // SQL queries need to be executed in the main thread
    QMetaObject::invokeMethod( this, "artistsSearch", Qt::BlockingQueuedConnection );

    QSet<QString> artistSet;
    artistSet.swap( m_artistsResult );

    return artistSet;
}

TrackList
FastForwardProvider::artistTracks( const QString &artistName )
{
    // SQL queries need to be executed in the main thread
    QMetaObject::invokeMethod( this, "artistTracksSearch", Qt::BlockingQueuedConnection,
                               Q_ARG( QString, artistName ) );

    TrackList artistTrackList;
    artistTrackList.swap( m_artistTracksResult );

    return artistTrackList;
}

void
FastForwardProvider::artistsSearch()
{
    QSqlDatabase db = QSqlDatabase::database( m_config["uid"].toString() );
    if( !db.isOpen() )
    {
        warning() << __PRETTY_FUNCTION__ << "could not open database connection:"
                  << db.lastError().text();
        return;
    }

    QSqlQuery query( "SELECT name FROM artist", db );
    query.setForwardOnly( true );

    while( query.next() )
        m_artistsResult.insert( query.value( 0 ).toString() );
}

void
FastForwardProvider::artistTracksSearch( const QString &artistName )
{
    QSqlDatabase db = QSqlDatabase::database( m_config["uid"].toString() );
    if( !db.isOpen() )
    {
        warning() << __PRETTY_FUNCTION__ << "could not open database connection:"
                  << db.lastError().text();
        return;
    }

    QSqlQuery query( db );
    query.setForwardOnly( true );

    query.prepare( "SELECT id FROM artist WHERE name = ?" );
    query.addBindValue( artistName );
    query.exec();

    if( !query.next() )
    {
        warning() << __PRETTY_FUNCTION__ << "could not find artist id:"
                  << query.lastError().text();
        return;
    }

    const int artistId = query.value( 0 ).toInt();

    query.prepare( "SELECT url FROM tags WHERE artist = ?" );
    query.addBindValue( artistId );
    query.exec();

    while ( query.next() )
    {
        const QString url = query.value( 0 ).toString();
        m_artistTracksResult
                << TrackPtr( new FastForwardTrack( url, m_config["uid"].toString() ) );
    }
}
