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
#include "AmarokEmbeddedSqlConnection.h"
#include "importers/ImporterSqlConnection.h"

using namespace StatSyncing;

AmarokProvider::AmarokProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
{
    if( config.value( QStringLiteral("embedded") ).toBool() )
    {
        QFileInfo mysqld( m_config.value( QStringLiteral("mysqlBinary") ).toString() );
        QDir datadir( m_config.value( QStringLiteral("dbPath") ).toString() );
        m_connection = ImporterSqlConnectionPtr(
                                     new AmarokEmbeddedSqlConnection( mysqld, datadir ) );
    }
    else
    {
        m_connection = ImporterSqlConnectionPtr( new ImporterSqlConnection(
             QStringLiteral("MYSQL"),
             m_config.value( QStringLiteral("dbHost") ).toString(),
             m_config.value( QStringLiteral("dbPort") ).toUInt(),
             m_config.value( QStringLiteral("dbName") ).toString(),
             m_config.value( QStringLiteral("dbUser") ).toString(),
             m_config.value( QStringLiteral("dbPass") ).toString()
        ) );
    }
}

AmarokProvider::~AmarokProvider()
{
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
    return Meta::valRating | Meta::valFirstPlayed | Meta::valLastPlayed
            | Meta::valPlaycount | Meta::valLabel;
}

QSet<QString>
AmarokProvider::artists()
{
    QSet<QString> result;
    for( const QVariantList &row : m_connection->query( QStringLiteral("SELECT name FROM artists") ) )
        result.insert( row[0].toString() );

    return result;
}

TrackList
AmarokProvider::artistTracks( const QString &artistName )
{
    const QString query = QStringLiteral("SELECT t.url, t.title, ar.name, al.name, c.name, y.name, "
                          "t.tracknumber, t.discnumber, s.rating, s.createdate, "
                          "s.accessdate, s.playcount "
                          "FROM tracks t "
                          "INNER JOIN artists     ar ON ar.id  = t.artist "
                          "LEFT  JOIN albums      al ON al.id  = t.album "
                          "LEFT  JOIN composers   c  ON c.id   = t.composer "
                          "LEFT  JOIN years       y  ON y.id   = t.year "
                          "LEFT  JOIN statistics  s  ON s.id   = t.id "
                          "WHERE ar.name = :artist");

    QVariantMap bindValues;
    bindValues.insert( QStringLiteral(":artist"), artistName );

    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valArtist
           << Meta::valAlbum << Meta::valComposer << Meta::valYear << Meta::valTrackNr
           << Meta::valDiscNr << Meta::valRating << Meta::valFirstPlayed
           << Meta::valLastPlayed << Meta::valPlaycount;

    TrackList result;
    for( const QVariantList &row : m_connection->query( query, bindValues ) )
    {
        const qint64 urlId = row[0].toInt();

        // Add one to i in query.value(), because the first value is a url id
        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata.insert( fields[i], row[i + 1] );

        const QString lblQuery = QStringLiteral("SELECT l.label FROM labels l "
                                 "INNER JOIN urls_labels ul ON ul.label = l.id "
                                 "WHERE ul.url = :url");
        QVariantMap lblBindValues;
        lblBindValues.insert( QStringLiteral(":url"), urlId );

        QSet<QString> labels;
        for( const QVariantList &lbl : m_connection->query( lblQuery, lblBindValues ) )
            labels.insert( lbl[0].toString() );

        result << TrackPtr( new AmarokTrack( urlId, m_connection, metadata, labels ) );
    }

    return result;
}
