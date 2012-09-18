/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "Statistics.h"

#include <QDateTime>

using namespace Meta;

Statistics::~Statistics()
{
}

double
Statistics::score() const
{
    return 0.0;
}

void
Statistics::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
Statistics::rating() const
{
    return 0;
}

void
Statistics::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

QDateTime
Statistics::lastPlayed() const
{
    return QDateTime();
}

void
Statistics::setLastPlayed( const QDateTime &date )
{
    Q_UNUSED( date )
}

QDateTime
Statistics::firstPlayed() const
{
    return QDateTime();
}

void
Statistics::setFirstPlayed( const QDateTime &date )
{
    Q_UNUSED( date )
}

int
Statistics::playCount() const
{
    return 0;
}

void
Statistics::setPlayCount( int newPlayCount )
{
    Q_UNUSED( newPlayCount )
}

void
Statistics::beginUpdate()
{
}

void
Statistics::endUpdate()
{
}
