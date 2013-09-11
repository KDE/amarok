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

#include <QSqlQuery>

using namespace StatSyncing;

BansheeProvider::BansheeProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterSqlProvider( setDbDriver( config ), importer )
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
BansheeProvider::getArtists( QSqlDatabase db )
{
    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.exec( "SELECT Name FROM coreartists" );

    QSet<QString> result;
    while( query.next() )
        result.insert( query.value( 0 ).toString() );

    return result;
}

TrackList
BansheeProvider::getArtistTracks( const QString &artistName, QSqlDatabase db )
{
    QSqlQuery query( db );
    query.setForwardOnly( true );
    // Due to Banshee's peculiar track info storage, to avoid massive amount of confusion
    // we only take tracks from PrimarySource: MusicLibrarySource-Library (always ID 1)
    query.prepare( "SELECT TrackID, TRIM(t.Title), ar.Name, al.Title, TRIM(t.Composer), "
                   "t.Year, t.TrackNumber, t.Disc, t.Rating, t.LastPlayedStamp, "
                   "t.PlayCount "
                   "FROM coretracks t "
                   "INNER JOIN coreartists ar USING(ArtistID) "
                   "LEFT JOIN corealbums al USING(AlbumID) "
                   "WHERE ar.Name = ? AND t.PrimarySourceID = 1" );
    query.addBindValue( artistName );
    query.exec();

    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valArtist
           << Meta::valAlbum << Meta::valComposer << Meta::valYear << Meta::valTrackNr
           << Meta::valDiscNr << Meta::valRating << Meta::valLastPlayed
           << Meta::valPlaycount;

    TrackList result;
    while ( query.next() )
    {
        const qint64 trackId = query.value( 0 ).toLongLong();

        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata.insert( fields[i], query.value( i + 1 ) );

        result << TrackPtr( new BansheeTrack( ImporterSqlProviderPtr( this ), trackId,
                                              metadata ) );
    }

    return result;
}

QVariantMap
BansheeProvider::setDbDriver( const QVariantMap &config )
{
    QVariantMap cfg( config );
    cfg.insert( "dbDriver", "QSQLITE" );
    return cfg;
}
