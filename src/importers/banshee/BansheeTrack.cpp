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

#include "BansheeTrack.h"

#include <QSqlQuery>
#include <QStringList>

using namespace StatSyncing;

BansheeTrack::BansheeTrack( const ImporterSqlProviderPtr &provider, const qint64 trackId,
                            const Meta::FieldHash &metadata )
    : ImporterSqlTrack( provider, metadata )
    , m_trackId( trackId )
{
}

BansheeTrack::~BansheeTrack()
{
}

int
BansheeTrack::rating() const
{
    return ImporterSqlTrack::rating() * 2;
}

void
BansheeTrack::setRating( int rating )
{
    ImporterSqlTrack::setRating( (rating + 1) / 2 );
}

void
BansheeTrack::sqlCommit( QSqlDatabase db, const QSet<qint64> &fields )
{
    QStringList updates;
    if( fields.contains( Meta::valLastPlayed ) )
        updates << "LastPlayedStamp = :lastplayed";
    if( fields.contains( Meta::valRating ) )
        updates << "Rating = :rating";
    if( fields.contains( Meta::valPlaycount ) )
        updates << "PlayCount = :playcount";

    if( !updates.empty() )
    {
        db.transaction();
        QSqlQuery query( db );

        query.prepare( "UPDATE coretracks SET "+updates.join(",")+"WHERE TrackID = :id" );
        query.bindValue( ":lastplayed", m_statistics.value( Meta::valLastPlayed ) );
        query.bindValue( ":rating", m_statistics.value( Meta::valRating ) );
        query.bindValue( ":playcount", m_statistics.value( Meta::valPlaycount ) );
        query.bindValue( ":id", m_trackId );

        if( !query.exec() )
        {
            db.rollback();
            return;
        }

        db.commit();
    }
}
