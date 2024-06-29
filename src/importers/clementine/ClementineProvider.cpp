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

#include "ClementineProvider.h"

#include "ClementineTrack.h"
#include "importers/ImporterSqlConnection.h"

using namespace StatSyncing;

ClementineProvider::ClementineProvider( const QVariantMap &config,
                                        ImporterManager *importer )
    : ImporterProvider( config, importer )
    , m_connection( new ImporterSqlConnection( config.value( QStringLiteral("dbPath") ).toString() ) )
{
}

ClementineProvider::~ClementineProvider()
{
}

qint64
ClementineProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum | Meta::valComposer
            | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
}

qint64
ClementineProvider::writableTrackStatsData() const
{
    return Meta::valLastPlayed | Meta::valRating | Meta::valPlaycount;
}

QSet<QString>
ClementineProvider::artists()
{
    m_connection->query( QStringLiteral("SELECT DISTINCT(artist) FROM songs") );

    QSet<QString> result;
    for( const QVariantList &row : m_connection->query( QStringLiteral("SELECT DISTINCT(artist) FROM songs") ) )
        result.insert( row[0].toString() );

    return result;
}

TrackList
ClementineProvider::artistTracks( const QString &artistName )
{
    const QString query = QStringLiteral("SELECT filename, title, artist, album, composer, year, track, "
            "disc, rating, lastplayed, playcount FROM songs WHERE artist = :artist");

    QVariantMap bindValues;
    bindValues.insert( QStringLiteral(":artist"), artistName );

    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valArtist
           << Meta::valAlbum << Meta::valComposer << Meta::valYear << Meta::valTrackNr
           << Meta::valDiscNr << Meta::valRating << Meta::valLastPlayed
           << Meta::valPlaycount;

    TrackList result;
    for( const QVariantList &row : m_connection->query( query, bindValues ) )
    {
        const QVariant &filename = row[0];

        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata.insert( fields[i], row[i + 1] );

        result << TrackPtr( new ClementineTrack( filename, m_connection, metadata ) );
    }

    return result;
}
