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

#include "AmarokTrack.h"

#include "core/support/Debug.h"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

using namespace StatSyncing;

AmarokTrack::AmarokTrack( const Meta::FieldHash &metadata, const QSet<QString> &labels )
    : m_metadata( metadata )
    , m_labels( labels )
{
}

AmarokTrack::~AmarokTrack()
{
}

QString
AmarokTrack::name() const
{
    return m_metadata[Meta::valTitle].toString();
}

QString
AmarokTrack::album() const
{
    return m_metadata[Meta::valAlbum].toString();
}

QString
AmarokTrack::artist() const
{
    return m_metadata[Meta::valArtist].toString();
}

QString
AmarokTrack::composer() const
{
    return m_metadata[Meta::valComposer].toString();
}

int
AmarokTrack::year() const
{
    return m_metadata[Meta::valYear].toInt();
}

int
AmarokTrack::trackNumber() const
{
    return m_metadata[Meta::valTrackNr].toInt();
}

int
AmarokTrack::discNumber() const
{
    return m_metadata[Meta::valDiscNr].toInt();
}

int
AmarokTrack::rating() const
{
    return m_metadata[Meta::valRating].toInt();
}

QDateTime
AmarokTrack::firstPlayed() const
{
    const QVariant &t = m_metadata[Meta::valFirstPlayed];
    return t.toUInt() == 0 ? QDateTime() : QDateTime::fromTime_t( t.toUInt() );
}

QDateTime
AmarokTrack::lastPlayed() const
{
    const QVariant &t = m_metadata[Meta::valLastPlayed];
    return t.toUInt() == 0 ? QDateTime() : QDateTime::fromTime_t( t.toUInt() );
}

int
AmarokTrack::playCount() const
{
    return m_metadata[Meta::valPlaycount].toInt();
}

QSet<QString>
AmarokTrack::labels() const
{
    return m_labels;
}
