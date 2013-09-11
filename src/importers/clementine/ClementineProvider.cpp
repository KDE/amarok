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

#include <QSqlQuery>

using namespace StatSyncing;

ClementineProvider::ClementineProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterSqlProvider( setDbDriver( config ), importer )
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
ClementineProvider::getArtists( QSqlDatabase db )
{
    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.exec( "SELECT DISTINCT(artist) FROM songs" );

    QSet<QString> result;
    while( query.next() )
        result.insert( query.value( 0 ).toString() );

    return result;
}

TrackList
ClementineProvider::getArtistTracks( const QString &artistName, QSqlDatabase db )
{
    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.prepare( "SELECT filename, title, artist, album, composer, year, track, disc, "
                   "rating, lastplayed, playcount "
                   "FROM songs "
                   "WHERE artist = ?" );
    query.addBindValue( artistName );
    query.exec();

    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valArtist
           << Meta::valAlbum << Meta::valComposer << Meta::valYear << Meta::valTrackNr
           << Meta::valDiscNr << Meta::valRating << Meta::valLastPlayed
           << Meta::valPlaycount;

    TrackList result;
    while ( query.next() )
    {
        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata.insert( fields[i], query.value( i + 1 ) );

        result << TrackPtr( new ClementineTrack( ImporterSqlProviderPtr( this ),
                                                 query.value( 0 ), metadata ) );
    }

    return result;
}

QVariantMap
ClementineProvider::setDbDriver( const QVariantMap &config )
{
    QVariantMap cfg( config );
    cfg.insert( "dbDriver", "QSQLITE" );
    return cfg;
}
