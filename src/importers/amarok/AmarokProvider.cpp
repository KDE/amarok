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

#include "MetaValues.h"
#include "statsyncing/SimpleTrack.h"

#include <QSqlQuery>

using namespace StatSyncing;

AmarokProvider::AmarokProvider( const QVariantMap &config, ImporterManager *importer )
    : ImporterSqlProvider( setDbDriver( config ), importer )
{
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
    //TODO: Write capabilities
    return 0;
}

QSet<QString>
AmarokProvider::getArtists( QSqlDatabase db )
{
    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.exec( "SELECT name FROM artists" );

    QSet<QString> result;
    while( query.next() )
        result.insert( query.value( 0 ).toString() );

    return result;
}

TrackList
AmarokProvider::getArtistTracks( const QString &artistName, QSqlDatabase db )
{
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

    const QList<qint64> fields = QList<qint64>() << Meta::valTitle << Meta::valArtist
           << Meta::valAlbum << Meta::valComposer << Meta::valYear << Meta::valTrackNr
           << Meta::valDiscNr << Meta::valRating << Meta::valFirstPlayed
           << Meta::valLastPlayed << Meta::valPlaycount;

    TrackList result;
    while ( query.next() )
    {
        const qint64 urlId = query.value( 0 ).toInt();

        // Add one to i in query.value(), because the first value is a url id
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

        result << TrackPtr( new SimpleTrack( metadata, labels ) );
    }

    return result;
}

QVariantMap
AmarokProvider::setDbDriver( const QVariantMap &config )
{
    QVariantMap cfg( config );
    cfg.insert( "dbDriver", "QMYSQL" );
    return cfg;
}
