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

#include "ITunesTrack.h"

using namespace StatSyncing;

ITunesTrack::ITunesTrack( const Meta::FieldHash &metadata )
    : SimpleTrack( metadata )
{
}

ITunesTrack::~ITunesTrack()
{
}

int
ITunesTrack::rating() const
{
    return m_metadata.value( Meta::valRating ).toInt() / 10;
}

QDateTime
ITunesTrack::lastPlayed() const
{
    return QDateTime::fromString( m_metadata.value( Meta::valLastPlayed ).toString(),
                                  "yyyy'-'MM'-'dd'T'hh':'mm':'ss'Z'" ).toLocalTime();
}
