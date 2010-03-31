/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "MtpReadCapability.h"
#include "MtpHandler.h"

using namespace Handler;

MtpPlaylistCapability::MtpPlaylistCapability( Meta::MtpHandler *handler )
        : Handler::PlaylistCapability()
        , m_handler( handler )
{
}

void
MtpPlaylistCapability::prepareToParsePlaylists()
{
    m_handler->prepareToParsePlaylists();
}

bool
MtpPlaylistCapability::isEndOfParsePlaylistsList()
{
    return m_handler->isEndOfParsePlaylistsList();
}

void
MtpPlaylistCapability::prepareToParseNextPlaylist()
{
    m_handler->prepareToParseNextPlaylist();
}

void
MtpPlaylistCapability::nextPlaylistToParse()
{
    m_handler->nextPlaylistToParse();
}

bool
MtpPlaylistCapability::shouldNotParseNextPlaylist()
{
    return m_handler->shouldNotParseNextPlaylist();
}

void
MtpPlaylistCapability::prepareToParsePlaylistTracks()
{
    m_handler->prepareToParsePlaylistTracks();
}

bool
MtpPlaylistCapability::isEndOfParsePlaylist()
{
    return m_handler->isEndOfParsePlaylist();
}

void
MtpPlaylistCapability::prepareToParseNextPlaylistTrack()
{
    m_handler->prepareToParseNextPlaylistTrack();
}

void
MtpPlaylistCapability::nextPlaylistTrackToParse()
{
    m_handler->nextPlaylistTrackToParse();
}

Meta::MediaDeviceTrackPtr
MtpPlaylistCapability::libGetTrackPtrForTrackStruct()
{
    return m_handler->libGetTrackPtrForTrackStruct();
}

QString
MtpPlaylistCapability::libGetPlaylistName()
{
    return m_handler->libGetPlaylistName();
}

void
MtpPlaylistCapability::savePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist, const QString& name )
{
    m_handler->libSavePlaylist( playlist, name );
}

void
MtpPlaylistCapability::deletePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_handler->deletePlaylist( playlist );
}

void
MtpPlaylistCapability::renamePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_handler->renamePlaylist( playlist );
}

#include "MtpPlaylistCapability.moc"
