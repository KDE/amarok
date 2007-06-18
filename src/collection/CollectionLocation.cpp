/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "CollectionLocation.h"

#include "collection.h"

CollectionLocation::CollectionLocation()
{
    //nothing to do
}

CollectionLocation::~CollectionLocation()
{
    //nothing to do
}

QString
CollectionLocation::prettyLocation() const
{
    return QString();
}

bool
CollectionLocation::isWriteable() const
{
    return false;
}

void
CollectionLocation::prepareCopy( Meta::TrackPtr track, CollectionLocation *destination )
{
    Meta::TrackList list;
    list.append( track );
    prepareCopy( list, destination );
}


void
CollectionLocation::prepareCopy( const Meta::TrackList &tracks, CollectionLocation *destination )
{
    if( !destination->isWriteable() )
        return;
    m_destination = destination;
    setupConnections();
    KUrl::List urls = getKIOCopyableUrls( tracks );
    emit startCopy( urls, false );
}

void
CollectionLocation::prepareMove( Meta::TrackPtr track, CollectionLocation *destination )
{
    Meta::TrackList list;
    list.append( track );
    prepareMove( list, destination );
}

void
CollectionLocation::prepareMove( const Meta::TrackList &tracks, CollectionLocation *destination )
{
    if( !destination->isWriteable() )
        return;
    m_destination = destination;
    setupConnections();
    KUrl::List urls = getKIOCopyableUrls( tracks );
    emit startCopy( urls, true );
}

bool
CollectionLocation::remove( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    return false;
}

KUrl::List
CollectionLocation::getKIOCopyableUrls( const Meta::TrackList &tracks )
{
    KUrl::List urls;
    foreach( Meta::TrackPtr track, tracks )
        if( track->isPlayable() )
            urls.append( track->playableUrl() );

    return urls;
}

void
CollectionLocation::copyUrlsToCollection( const KUrl::List &sources )
{
    //reimplement in implementations which are writeable
    Q_UNUSED( sources )
    slotCopyOperationFinished();
}

void
CollectionLocation::slotCopyOperationFinished()
{
    emit finishCopy( m_removeSources );
}

void
CollectionLocation::slotStartCopy( const KUrl::List &sources, bool removeSources )
{
    m_removeSources = removeSources;
    copyUrlsToCollection( sources );
}

void
CollectionLocation::slotFinishCopy( bool removeSources )
{
    if( removeSources )
        removeSourceTracks( m_sourceTracks );
    m_sourceTracks.clear();
    m_destination->deleteLater();
    this->deleteLater();
}

void
CollectionLocation::setupConnections()
{
    connect( this, SIGNAL( startCopy( KUrl::List, bool ) ),
             m_destination, SLOT( slotStartCopy( KUrl::List, bool ) ) );
    connect( m_destination, SIGNAL( finishCopy( bool ) ),
             this, SLOT( slotFinishCopy( bool ) ) );
}

void
CollectionLocation::removeSourceTracks( const Meta::TrackList &tracks )
{
    Meta::TrackList notDeletableTracks;
    foreach( Meta::TrackPtr track, tracks )
    {
        if( !remove( track ) )
            notDeletableTracks.append( track );
    }
    //TODO inform user about tracks which were not deleted
}

#include "CollectionLocation.moc"
