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

#include "StatisticsProvider.h"

#include "Amarok.h"
#include "Debug.h"

Statistics::StatisticsProvider::StatisticsProvider()
        : m_score( 0.0 )
        , m_rating( 0 )
        , m_playCount( 0 )
{
}

Statistics::StatisticsProvider::~StatisticsProvider()
{
}

void
Statistics::StatisticsProvider::played( double playedFraction, Meta::TrackPtr track )
{
    DEBUG_BLOCK

    debug() << "called with playedFraction = " << playedFraction;

    m_lastPlayed = QDateTime::currentDateTime();
    if( !m_firstPlayed.isValid() )
    {
        m_firstPlayed = QDateTime::currentDateTime();
    }

    bool doUpdate = false;
    int oldPlayCount = m_playCount;

    if( track->length() < 30000 && playedFraction == 1.0 )
        doUpdate = true;
    if( playedFraction >= 0.5 && track->length() >= 30000 ) //song >= 30 seconds and at least half played
        doUpdate = true;
    if( playedFraction * track->length() > 240000 )
        doUpdate = true;

    if( doUpdate )
    {
        m_playCount++;
        if( !m_firstPlayed.isValid() )
            setFirstPlayed( QDateTime::currentDateTime() );
        setLastPlayed( QDateTime::currentDateTime() );
    }

    m_score = Amarok::computeScore( m_score, oldPlayCount, playedFraction );
    save();
}

QDateTime
Statistics::StatisticsProvider::firstPlayed() const
{
    return m_firstPlayed;
}

void
Statistics::StatisticsProvider::setFirstPlayed( const QDateTime &dt )
{
    m_firstPlayed = dt;
    save();
}

QDateTime
Statistics::StatisticsProvider::lastPlayed() const
{
    return m_lastPlayed;
}

void
Statistics::StatisticsProvider::setLastPlayed( const QDateTime &dt )
{
    m_lastPlayed = dt;
    save();
}

int
Statistics::StatisticsProvider::playCount() const
{
    return m_playCount;
}

void
Statistics::StatisticsProvider::setPlayCount( int playCount )
{
    m_playCount = playCount;
    save();
}

int
Statistics::StatisticsProvider::rating() const
{
    return m_rating;
}

double
Statistics::StatisticsProvider::score() const
{
    return m_score;
}

void
Statistics::StatisticsProvider::setRating( int newRating )
{
    m_rating = newRating;
    save();
}

void
Statistics::StatisticsProvider::setScore( double newScore )
{
    m_score = newScore;
    save();
}
