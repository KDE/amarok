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

#include "BansheeProvider.h"

#include "BansheeTrack.h"
#include "importers/ImporterSqlConnection.h"

using namespace StatSyncing;

BansheeProvider::BansheeProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterProvider( config, importer )
    , m_connection( new ImporterSqlConnection( m_config.value( QStringLiteral("dbPath") ).toString() ) )
{
}

BansheeProvider::~BansheeProvider()
{
}

qint64
BansheeProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum | Meta::valComposer
            | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
}

qint64
BansheeProvider::writableTrackStatsData() const
{
    return Meta::valRating | Meta::valLastPlayed | Meta::valPlaycount;
}

QSet<QString>
BansheeProvider::artists()
{
    QSet<QString> result;
    for( const QVariantList &row : m_connection->query( QStringLiteral("SELECT Name FROM coreartists") ) )
        result.insert( row[0].toString() );

    return result;
}

TrackList
BansheeProvider::artistTracks( const QString &artistName )
{
    // Due to Banshee's peculiar track info storage, to avoid massive amount of confusion
    // we only take tracks from PrimarySource: MusicLibrarySource-Library (always ID 1)
    const QString query = QStringLiteral("SELECT TrackID, TRIM(t.Title), ar.Name, al.Title, "
            "TRIM(t.Composer), t.Year, t.TrackNumber, t.Disc, t.Rating, "
            "t.LastPlayedStamp, t.PlayCount "
            "FROM coretracks t "
            "INNER JOIN coreartists ar USING(ArtistID) "
            "LEFT JOIN corealbums al USING(AlbumID) "
            "WHERE ar.Name = :artist AND t.PrimarySourceID = 1");

    QVariantMap bindValues;
    bindValues.insert( QStringLiteral(":artist"), artistName );

    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valArtist
           << Meta::valAlbum << Meta::valComposer << Meta::valYear << Meta::valTrackNr
           << Meta::valDiscNr << Meta::valRating << Meta::valLastPlayed
           << Meta::valPlaycount;

    TrackList result;
    for( const QVariantList &row : m_connection->query( query, bindValues ) )
    {
        const qint64 trackId = row[0].toLongLong();

        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata.insert( fields[i], row[i + 1] );

        result << TrackPtr( new BansheeTrack( trackId, m_connection, metadata ) );
    }

    return result;
}
