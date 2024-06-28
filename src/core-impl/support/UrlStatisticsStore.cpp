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

#include "UrlStatisticsStore.h"

#include <core/storage/SqlStorage.h>
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/storage/StorageManager.h"

UrlStatisticsStore::UrlStatisticsStore( Meta::Track *track, const QString &permanentUrl )
    : PersistentStatisticsStore( track )
    , m_permanentUrl( permanentUrl )
{
    if( m_permanentUrl.isEmpty() )
        m_permanentUrl = track->uidUrl();
    auto sql = StorageManager::instance()->sqlStorage();
    if( !sql )
    {
        warning() << __PRETTY_FUNCTION__ << "could not get SqlStorage, aborting";
        return;
    }


    const QString query = QStringLiteral("SELECT firstplayed, lastplayed, score, rating, playcount FROM "
                          "statistics_permanent WHERE url = '%1'");
    QStringList result = sql->query( query.arg( sql->escape( m_permanentUrl ) ) );
    if( !result.isEmpty() )
    {
        m_firstPlayed = QDateTime::fromString( result.value( 0 ), s_sqlDateFormat );
        m_lastPlayed = QDateTime::fromString( result.value( 1 ), s_sqlDateFormat );
        m_score = result.value( 2 ).toDouble();
        m_rating = result.value( 3 ).toInt();
        m_playCount = result.value( 4 ).toInt();
    }
}

void
UrlStatisticsStore::save()
{
    auto sql = StorageManager::instance()->sqlStorage();
    if( !sql )
    {
        warning() << __PRETTY_FUNCTION__ << "could not get SqlStorage, aborting";
        return;
    }

    const QString check = QStringLiteral("SELECT COUNT(*) FROM statistics_permanent WHERE url = '%1'");
    QStringList rsCheck = sql->query( check.arg( sql->escape( m_permanentUrl ) ) );
    if( !rsCheck.isEmpty() )
    {
        QString sqlString;
        if( rsCheck.first().toInt() )
        {
            sqlString = QStringLiteral("UPDATE statistics_permanent SET firstplayed = '%1',lastplayed = '%2',"
                        "score = %3,rating = %4,playcount=%5 WHERE url = '%6'");
        }
        else
        {
            sqlString = QStringLiteral("INSERT INTO statistics_permanent(firstplayed,lastplayed,score,"
                        "rating,playcount,url) VALUE ('%1','%2',%3,%4,%5,'%6')");
        }
        sqlString = sqlString.arg( m_firstPlayed.toString( s_sqlDateFormat ),
                                   m_lastPlayed.toString( s_sqlDateFormat ),
                                   QString::number( m_score ),
                                   QString::number( m_rating ),
                                   QString::number( m_playCount ),
                                   sql->escape( m_permanentUrl ) );
        sql->query( sqlString );
    }
}
