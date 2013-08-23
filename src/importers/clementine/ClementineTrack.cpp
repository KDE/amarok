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

#include "ClementineTrack.h"

using namespace StatSyncing;

ClementineTrack::ClementineTrack( const Meta::FieldHash &metadata )
    : SimpleTrack( metadata )
{
}

ClementineTrack::~ClementineTrack()
{
}

int
ClementineTrack::year() const
{
    const int yr = SimpleTrack::year();
    return yr == -1 ? 0 : yr;
}

int
ClementineTrack::trackNumber() const
{
    const int tn = SimpleTrack::trackNumber();
    return tn == -1 ? 0 : tn;
}

int
ClementineTrack::discNumber() const
{
    const int dn = SimpleTrack::discNumber();
    return dn == -1 ? 0 : dn;
}

QDateTime
ClementineTrack::lastPlayed() const
{
    const int lp = m_metadata.value( Meta::valLastPlayed ).toInt();
    return lp == -1 ? QDateTime() : SimpleTrack::lastPlayed();
}

int
ClementineTrack::playCount() const
{
    const int pc = SimpleTrack::playCount();
    return pc == -1 ? 0 : pc;
}

int
ClementineTrack::rating() const
{
    return qRound( m_metadata.value( Meta::valRating ).toReal() * 10 );
}
