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

#include "MetaValues.h"

using namespace StatSyncing;

ITunesTrack::ITunesTrack( const QMap<qint64, QString> &metadata )
    : Track()
    , m_metadata( metadata )
{
}

ITunesTrack::~ITunesTrack()
{
}

QString
ITunesTrack::name() const
{
    return m_metadata[Meta::valTitle];
}

QString
ITunesTrack::album() const
{
    return m_metadata[Meta::valAlbum];
}

QString
ITunesTrack::artist() const
{
    return m_metadata[Meta::valArtist];
}

QString
ITunesTrack::composer() const
{
    return m_metadata[Meta::valComposer];
}

int
ITunesTrack::year() const
{
    return m_metadata[Meta::valYear].toInt();
}

int
ITunesTrack::trackNumber() const
{
    return m_metadata[Meta::valTrackNr].toInt();
}

int
ITunesTrack::discNumber() const
{
    return m_metadata[Meta::valDiscNr].toInt();
}

int
ITunesTrack::rating() const
{
    return m_metadata[Meta::valRating].toInt() / 10;
}

QDateTime
ITunesTrack::firstPlayed() const
{
    return QDateTime();
}

QDateTime
ITunesTrack::lastPlayed() const
{
    return QDateTime::fromString( m_metadata[Meta::valLastPlayed],
                                  "yyyy'-'MM'-'dd'T'hh':'mm':'ss'Z'" ).toLocalTime();
}

int
ITunesTrack::playCount() const
{
    return m_metadata[Meta::valPlaycount].toInt();
}

QSet<QString>
ITunesTrack::labels() const
{
    return QSet<QString>();
}
