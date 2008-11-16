/* This file is part of the KDE project
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "CollectionCapability"

#include "CollectionCapability.h"

#include "Debug.h"

#include "QueryMaker.h"

Meta::CollectionCapabilityHelper::CollectionCapabilityHelper( QueryMaker *qm )
    :  QObject(), m_querymaker ( qm ) {}

Meta::CollectionCapabilityHelper::CollectionCapabilityHelper( TrackList *tracklist )
    : QObject(), m_tracklist ( tracklist ) {}

Meta::CollectionCapabilityHelper::~CollectionCapabilityHelper()
{
    DEBUG_BLOCK
}

void
Meta::CollectionCapabilityHelper::newResultReady( QString collId, Meta::TrackList tracklist )
{
    emit tracklistReady( tracklist );
}

void
Meta::CollectionCapabilityHelper::runQuery()
{
    DEBUG_BLOCK
    m_querymaker->run();
    //deleteLater();
}

Meta::CollectionCapability::~CollectionCapability()
{
}

#include "CollectionCapability.moc"
