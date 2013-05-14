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

#include "MediaDeviceTrackEditor.h"

using namespace Meta;

MediaDeviceTrackEditor::MediaDeviceTrackEditor( MediaDeviceTrack *track )
    : Meta::TrackEditor()
    , m_inBatchUpdate( false )
    , m_track( track )
{
}

void
MediaDeviceTrackEditor::setAlbum( const QString &newAlbum )
{
    m_track->setAlbum( newAlbum );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setAlbumArtist( const QString &newAlbumArtist )
{
    m_track->setAlbumArtist( newAlbumArtist );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setArtist( const QString &newArtist )
{
    m_track->setArtist( newArtist );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setComposer( const QString &newComposer )
{
    m_track->setComposer( newComposer );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setGenre( const QString &newGenre )
{
    m_track->setGenre( newGenre );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setYear( int newYear )
{
    m_track->setYear( newYear );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setBpm( const qreal newBpm )
{
    m_track->setBpm( newBpm );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setTitle( const QString &newTitle )
{
    m_track->setTitle( newTitle );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setComment( const QString &newComment )
{
    m_track->setComment( newComment );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setTrackNumber( int newTrackNumber )
{
    m_track->setTrackNumber( newTrackNumber );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::setDiscNumber( int newDiscNumber )
{
    m_track->setDiscNumber( newDiscNumber );
    commitIfInNonBatchUpdate();
}

void
MediaDeviceTrackEditor::beginUpdate()
{
    m_inBatchUpdate = true;
}

void
MediaDeviceTrackEditor::endUpdate()
{
    m_inBatchUpdate = false;
    m_track->commitChanges();
}

void
MediaDeviceTrackEditor::commitIfInNonBatchUpdate()
{
    if( m_inBatchUpdate )
        return;
    m_track->commitChanges();
}
