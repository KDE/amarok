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

Meta::StatisticsProvider::StatisticsProvider()
        : m_score( 0.0 )
        , m_rating( 0 )
        , m_playCount( 0 )
{
}

Meta::StatisticsProvider::~StatisticsProvider()
{
}

void
Meta::StatisticsProvider::played( double playedFraction )
{
    DEBUG_BLOCK

    debug() << "called with playedFraction = " << playedFraction;
    m_lastPlayed = QDateTime::currentDateTime();
    if( !m_firstPlayed.isValid() )
    {
        m_firstPlayed = QDateTime::currentDateTime();
    }
    m_playCount++;
    m_score = Amarok::computeScore( m_score, m_playCount, playedFraction );
    save();
}

QDateTime
Meta::StatisticsProvider::firstPlayed() const
{
    return m_firstPlayed;
}

void
Meta::StatisticsProvider::setFirstPlayed( const QDateTime &dt )
{
    m_firstPlayed = dt;
    save();
}

QDateTime
Meta::StatisticsProvider::lastPlayed() const
{
    return m_lastPlayed;
}

void
Meta::StatisticsProvider::setLastPlayed( const QDateTime &dt )
{
    m_lastPlayed = dt;
    save();
}

int
Meta::StatisticsProvider::playCount() const
{
    return m_playCount;
}

void
Meta::StatisticsProvider::setPlayCount( int playCount )
{
    m_playCount = playCount;
    save();
}

int
Meta::StatisticsProvider::rating() const
{
    return m_rating;
}

double
Meta::StatisticsProvider::score() const
{
    return m_score;
}

void
Meta::StatisticsProvider::setRating( int newRating )
{
    m_rating = newRating;
    save();
}

void
Meta::StatisticsProvider::setScore( double newScore )
{
    m_score = newScore;
    save();
}
