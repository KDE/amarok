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

#include "AmarokProvider.h"

#include "AmarokTrack.h"
#include "MetaValues.h"

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>

using namespace StatSyncing;

AmarokProvider::AmarokProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( "QMYSQL", m_config.value( "uid" ).toString() );
    db.setDatabaseName( m_config.value( "dbName" ).toString() );
    db.setHostName    ( m_config.value( "dbHost" ).toString() );
    db.setUserName    ( m_config.value( "dbUser" ).toString() );
    db.setPassword    ( m_config.value( "dbPass" ).toString() );
    db.setPort        ( m_config.value( "dbPort" ).toInt()    );
}

AmarokProvider::~AmarokProvider()
{
    QSqlDatabase::removeDatabase( m_config.value( "uid" ).toString() );
}

qint64
AmarokProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum | Meta::valComposer
            | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
}

qint64
AmarokProvider::writableTrackStatsData() const
{
    //TODO: Write capabilities
    return 0;
}

QSet<QString>
AmarokProvider::artists()
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
AmarokProvider::artistTracks( const QString &artistName )
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
AmarokProvider::artistsSearch()
{
    QSqlDatabase db = QSqlDatabase::database( m_config["uid"].toString() );
    if( !db.isOpen() )
        return;

    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.exec( "SELECT name FROM artists" );

    while( query.next() )
        m_artistsResult.insert( query.value( 0 ).toString() );
}

void
AmarokProvider::artistTracksSearch( const QString &artistName )
{
    QSqlDatabase db = QSqlDatabase::database( m_config["uid"].toString() );
    if( !db.isOpen() )
        return;

    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.prepare( "SELECT t.url, t.title, ar.name, al.name, c.name, y.name, "
                   "t.tracknumber, t.discnumber, s.rating, s.createdate, "
                   "s.accessdate, s.playcount "
                   "FROM tracks t "
                   "INNER JOIN artists     ar ON ar.id  = t.artist "
                   "LEFT  JOIN albums      al ON al.id  = t.album "
                   "LEFT  JOIN composers   c  ON c.id   = t.composer "
                   "LEFT  JOIN years       y  ON y.id   = t.year "
                   "LEFT  JOIN statistics  s  ON s.id   = t.id "
                   "LEFT  JOIN urls_labels ul ON ul.url = t.url "
                   "WHERE ar.name = ?" );
    query.addBindValue( artistName );
    query.exec();

    QList<qint64> fields;
    fields << Meta::valTitle << Meta::valArtist << Meta::valAlbum << Meta::valComposer
           << Meta::valYear << Meta::valTrackNr << Meta::valDiscNr << Meta::valRating
           << Meta::valFirstPlayed << Meta::valLastPlayed << Meta::valPlaycount;

    while ( query.next() )
    {
        const qint64 urlId = query.value( 0 ).toInt();

        // Add one to i in query.value(), because first value is a url id
        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata[fields[i]] = query.value( i + 1 );

        QSqlQuery lblQuery( db );
        lblQuery.setForwardOnly( true );
        lblQuery.prepare( "SELECT l.label "
                             "FROM labels l "
                             "INNER JOIN urls_labels ul ON ul.label = l.id "
                             "WHERE ul.url = ?");
        lblQuery.addBindValue( urlId );
        lblQuery.exec();

        QSet<QString> labels;
        while( lblQuery.next() )
            labels << lblQuery.value( 0 ).toString();

        m_artistTracksResult << TrackPtr( new AmarokTrack( metadata, labels ) );
    }
}
