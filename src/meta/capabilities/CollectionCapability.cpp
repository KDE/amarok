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
    :  QObject(), m_tracklist( 0 ), m_querymaker ( qm )
{
    DEBUG_BLOCK
}

Meta::CollectionCapabilityHelper::CollectionCapabilityHelper( TrackList *tracklist )
    : QObject(), m_tracklist ( tracklist ), m_querymaker( 0 ) {}

Meta::CollectionCapabilityHelper::~CollectionCapabilityHelper()
{
    DEBUG_BLOCK
}

void
Meta::CollectionCapabilityHelper::setAction( PopupDropperAction *action, const QObject *receiver, const char *method )
{
    DEBUG_BLOCK
    connect( action, SIGNAL( triggered() ), this, SLOT( runQuery() ) );
    connect( m_querymaker, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( newResultReady ( QString, Meta::TrackList ) ), Qt::QueuedConnection );
    connect( m_querymaker, SIGNAL( queryDone() ), this, SLOT( tracklistReadySlot() ), Qt::QueuedConnection );

    connect( this, SIGNAL( tracklistReady( Meta::TrackList ) ), receiver, method, Qt::QueuedConnection );

    // Make sure Helper is deleted in case no action gets called
    connect( action, SIGNAL( destroyed() ), this, SLOT( deleteLater() ) );

}

void
Meta::CollectionCapabilityHelper::newResultReady( QString collId, Meta::TrackList tracklist )
{
    Q_UNUSED( collId );
    DEBUG_BLOCK
    if( !m_tracklist )
        m_tracklist = new QList<Meta::TrackPtr>();
    *m_tracklist << tracklist;
    debug() << "m_tracklist size " << m_tracklist->size();
}

void
Meta::CollectionCapabilityHelper::runQuery()
{
    DEBUG_BLOCK
    // Stops the Helper from being destroyed while QM runs
    //disconnect( sender(), 0, 0, 0 );
    m_querymaker->run();
}

void
Meta::CollectionCapabilityHelper::tracklistReadySlot()
{
    emit tracklistReady( *m_tracklist );
}

Meta::CollectionCapability::~CollectionCapability()
{
}

#include "CollectionCapability.moc"
