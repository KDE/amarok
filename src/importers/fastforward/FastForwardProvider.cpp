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
    : ImporterSqlProvider( config, importer )
{
}

FastForwardProvider::~FastForwardProvider()
{
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
FastForwardProvider::getArtists( QSqlDatabase db )
{
    QSqlQuery query( db );
    query.setForwardOnly( true );
    query.prepare( "SELECT name FROM artist" );
    query.exec();

    QSet<QString> result;
    while( query.next() )
        result.insert( query.value( 0 ).toString() );

    return result;
}

TrackList
FastForwardProvider::getArtistTracks( const QString &artistName, QSqlDatabase db )
{
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

    TrackList result;
    while ( query.next() )
    {
        const QString trackUrl = query.value( 0 ).toString();

        Meta::FieldHash metadata;
        for( int i = 0; i < fields.size(); ++i )
            metadata.insert( fields[i], query.value( i + 1 ) );

        result << TrackPtr( new FastForwardTrack( metadata, trackUrl, m_connectionName ) );
    }

    return result;
}
