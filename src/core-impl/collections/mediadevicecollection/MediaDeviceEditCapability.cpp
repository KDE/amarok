/****************************************************************************************
 * Copyright (c) 2011 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "MediaDeviceEditCapability.h"

using namespace Meta;

MediaDeviceEditCapability::MediaDeviceEditCapability( MediaDeviceTrack *track )
    : Meta::TrackEditor()
    , m_inBatchUpdate( false )
    , m_track( track )
{
}

MediaDeviceEditCapability::~MediaDeviceEditCapability()
{
    // nothing to do
}

void
MediaDeviceEditCapability::setAlbum( const QString &newAlbum )
{
    m_track->setAlbum( newAlbum );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setAlbumArtist( const QString &newAlbumArtist )
{
    m_track->setAlbumArtist( newAlbumArtist );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setArtist( const QString &newArtist )
{
    m_track->setArtist( newArtist );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setComposer( const QString &newComposer )
{
    m_track->setComposer( newComposer );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setGenre( const QString &newGenre )
{
    m_track->setGenre( newGenre );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setYear( int newYear )
{
    m_track->setYear( newYear );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setBpm( const qreal newBpm )
{
    m_track->setBpm( newBpm );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setTitle( const QString &newTitle )
{
    m_track->setTitle( newTitle );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setComment( const QString &newComment )
{
    m_track->setComment( newComment );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setTrackNumber( int newTrackNumber )
{
    m_track->setTrackNumber( newTrackNumber );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::setDiscNumber( int newDiscNumber )
{
    m_track->setDiscNumber( newDiscNumber );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceEditCapability::beginUpdate()
{
    m_inBatchUpdate = true;
}

void
MediaDeviceEditCapability::endUpdate()
{
    m_inBatchUpdate = false;
    m_track->commitChanges();
}

void
MediaDeviceEditCapability::commitIfInNonBatchUpdate()
{
    if( m_inBatchUpdate )
        return;
    m_track->commitChanges();
}

#include "MediaDeviceEditCapability.moc"
