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

#include <QApplication>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

using namespace StatSyncing;

FastForwardProvider::FastForwardProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( m_config["dbDriver"].toString(), id() );

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
    QSqlDatabase::removeDatabase( id() );
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
    const Qt::ConnectionType connectionType =
            this->thread() == QCoreApplication::instance()->thread()
            ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

    QMetaObject::invokeMethod( this, "artistsSearch", connectionType );

    QSet<QString> artistSet;
    artistSet.swap( m_artistsResult );

    return artistSet;
}

TrackList
FastForwardProvider::artistTracks( const QString &artistName )
{
    // SQL queries need to be executed in the main thread
    const Qt::ConnectionType connectionType =
            this->thread() == QCoreApplication::instance()->thread()
            ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

    QMetaObject::invokeMethod( this, "artistTracksSearch", connectionType,
                               Q_ARG( QString, artistName ) );

    TrackList artistTrackList;
    artistTrackList.swap( m_artistTracksResult );

    return artistTrackList;
}

void
FastForwardProvider::artistsSearch()
{
    QSqlDatabase db = QSqlDatabase::database( id() );
    if( !db.isOpen() )
        return;

    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.prepare( "SELECT name FROM artist" );
    query.exec();

    while( query.next() )
        m_artistsResult.insert( query.value( 0 ).toString() );
}

void
FastForwardProvider::artistTracksSearch( const QString &artistName )
{
    QSqlDatabase db = QSqlDatabase::database( id() );
    if( !db.isOpen() )
        return;

    QSqlQuery query( db );
    query.setForwardOnly( true );

    query.prepare( "SELECT t.url, t.title, al.name, ar.name, c.name, y.name, t.track, "
                   "t.discnumber "
                   "FROM tags t "
                   "INNER JOIN artist ar ON ar.id = t.artist "
                   "LEFT JOIN album al ON al.id = t.album "
                   "LEFT JOIN composer c ON c.id = t.composer "
                   "LEFT JOIN year y ON y.id = t.year "
                   "WHERE ar.name = ?" );
    query.addBindValue( artistName );
    query.exec();

    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valAlbum
                                 << Meta::valArtist << Meta::valComposer
                                 << Meta::valYear << Meta::valTrackNr << Meta::valDiscNr;

    while ( query.next() )
    {
        const QString trackUrl = query.value( 0 ).toString();

        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata.insert( fields[i], query.value( i + 1 ) );

        m_artistTracksResult
                << TrackPtr( new FastForwardTrack( metadata, trackUrl, id() ) );
    }
}
