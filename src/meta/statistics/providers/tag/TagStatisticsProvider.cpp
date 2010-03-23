/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "TagStatisticsProvider.h"

#include "collection/CollectionManager.h"
#include "collection/SqlStorage.h"

TagStatisticsProvider::TagStatisticsProvider( const QString &name, const QString &artist, const QString &album )
        : StatisticsProvider()
        , m_name( name )
        , m_artist( artist )
        , m_album( album )
{
    SqlStorage *sql = CollectionManager::instance()->sqlStorage();

    const QString query = "SELECT firstPlayed, lastPlayed, score, rating, playcount FROM "
                          "statistics_tag WHERE name = '%1' AND artist = '%2' AND album = '%3'";
    QStringList result = sql->query( query.arg( sql->escape( name ),
                                                sql->escape( artist ),
                                                sql->escape( album ) ) );
    if( !result.isEmpty() )
    {
        m_firstPlayed = QDateTime::fromString( result.value( 0 ), "yy-MM-dd hh:mm:ss" );
        m_lastPlayed = QDateTime::fromString( result.value( 1 ), "yy-MM-dd hh:mm:ss" );
        m_score = result.value( 2 ).toDouble();
        m_rating = result.value( 3 ).toInt();
        m_playCount = result.value( 4 ).toInt();
    }
}

void
TagStatisticsProvider::save()
{
    SqlStorage *sql = CollectionManager::instance()->sqlStorage();

    const QString check = "SELECT COUNT(*) FROM statistics_tag WHERE name = '%1' "
                          "AND artist = '%2' AND album = '%3'";
    QStringList rsCheck = sql->query( check.arg( sql->escape( m_name ),
                                                 sql->escape( m_artist ),
                                                 sql->escape( m_album ) ) );
    if( !rsCheck.isEmpty() )
    {
        QString sqlString;
        if( rsCheck.first().toInt() )
        {
            sqlString = "UPDATE statistics_tag SET firstPlayed = '%1',lastPlayed = '%2',"
                        "score = %3,rating = %4,playcount=%5 WHERE name = '%6' "
                        "AND artist = '%7' AND album = '%8'";
        }
        else
        {
            sqlString = "INSERT INTO statistics_tag(firstPlayed,lastPlayed,score,"
                        "rating,playcount,name,artist,album) "
                        "VALUE ('%1','%2',%3,%4,%5,'%6','%7','%8')";
        }
        sqlString = sqlString.arg( m_firstPlayed.toString( "yy-MM-dd hh:mm:ss" ),
                                   m_lastPlayed.toString( "yy-MM-dd hh:mm:ss" ),
                                   QString::number( m_score ),
                                   QString::number( m_rating ),
                                   QString::number( m_playCount ),
                                   sql->escape( m_name ),
                                   sql->escape( m_artist ),
                                   sql->escape( m_album ) );
        sql->query( sqlString );
    }
}

