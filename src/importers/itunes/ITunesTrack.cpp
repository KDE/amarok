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

ITunesTrack::ITunesTrack()
    : Track()
{
}

ITunesTrack::~ITunesTrack()
{
}

QString
ITunesTrack::name() const
{
    return QString();
}

QString
ITunesTrack::album() const
{
    return QString();
}

QString
ITunesTrack::artist() const
{
    return QString();
}

QString
ITunesTrack::composer() const
{
    return QString();
}

int
ITunesTrack::year() const
{
    return 0;
}

int
ITunesTrack::trackNumber() const
{
    return 0;
}

int
ITunesTrack::discNumber() const
{
    return 0;
}

int
ITunesTrack::rating() const
{
    return 0;
}

QDateTime
ITunesTrack::firstPlayed() const
{
    return QDateTime();
}

QDateTime
ITunesTrack::lastPlayed() const
{
    return QDateTime();
}

int
ITunesTrack::playCount() const
{
    return 0;
}

QSet<QString>
ITunesTrack::labels() const
{
    return QSet<QString>();
}
