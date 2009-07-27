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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "IpodReadCapability.h"
#include "IpodHandler.h"

using namespace Handler;

IpodPlaylistCapability::IpodPlaylistCapability( Meta::IpodHandler *handler )
        : Handler::PlaylistCapability()
        , m_handler( handler )
{
}

void
IpodPlaylistCapability::prepareToParsePlaylists()
{
    m_handler->prepareToParsePlaylists();
}

bool
IpodPlaylistCapability::isEndOfParsePlaylistsList()
{
    return m_handler->isEndOfParsePlaylistsList();
}

void
IpodPlaylistCapability::prepareToParseNextPlaylist()
{
    m_handler->prepareToParseNextPlaylist();
}

void
IpodPlaylistCapability::nextPlaylistToParse()
{
    m_handler->nextPlaylistToParse();
}

bool
IpodPlaylistCapability::shouldNotParseNextPlaylist()
{
    return m_handler->shouldNotParseNextPlaylist();
}

void
IpodPlaylistCapability::prepareToParsePlaylistTracks()
{
    m_handler->prepareToParsePlaylistTracks();
}

bool
IpodPlaylistCapability::isEndOfParsePlaylist()
{
    return m_handler->isEndOfParsePlaylist();
}

void
IpodPlaylistCapability::prepareToParseNextPlaylistTrack()
{
    m_handler->prepareToParseNextPlaylistTrack();
}

void
IpodPlaylistCapability::nextPlaylistTrackToParse()
{
    m_handler->nextPlaylistTrackToParse();
}

Meta::MediaDeviceTrackPtr
IpodPlaylistCapability::libGetTrackPtrForTrackStruct()
{
    return m_handler->libGetTrackPtrForTrackStruct();
}

QString
IpodPlaylistCapability::libGetPlaylistName()
{
    return m_handler->libGetPlaylistName();
}

void
IpodPlaylistCapability::savePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name )
{
    m_handler->libSavePlaylist( playlist, name );
}

void
IpodPlaylistCapability::deletePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    m_handler->deletePlaylist( playlist );
}

void
IpodPlaylistCapability::renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    m_handler->renamePlaylist( playlist );
}

void
IpodPlaylistCapability::setAssociatePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    m_handler->setAssociatePlaylist( playlist );
}



#include "IpodPlaylistCapability.moc"
