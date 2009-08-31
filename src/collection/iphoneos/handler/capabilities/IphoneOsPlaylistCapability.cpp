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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "IphoneOs::PlaylistCapability"

#include "IphoneOsPlaylistCapability.h"
#include "../IphoneOsHandler.h"

using namespace Handler;

IphoneOsPlaylistCapability::IphoneOsPlaylistCapability( Meta::IphoneOsHandler *handler )
        : Handler::PlaylistCapability()
        , m_handler( handler )
{
    DEBUG_BLOCK
}

void
IphoneOsPlaylistCapability::prepareToParsePlaylists()
{
    QString sql = "SELECT pid FROM container";
    m_playlistIds = m_handler->query(m_handler->m_libraryDb, sql);
    m_currentPlaylistIndex = 0;
    debug() << "playlist ids: " << m_playlistIds;

    prepareToParseNextPlaylist();
}

bool
IphoneOsPlaylistCapability::isEndOfParsePlaylistsList()
{
    return m_currentPlaylistIndex >= m_playlistIds.size()-1;
}

void
IphoneOsPlaylistCapability::prepareToParseNextPlaylist()
{
    DEBUG_BLOCK

    m_currentPlaylist = m_playlistIds[m_currentPlaylistIndex];
    QString sql = QString("SELECT name FROM container WHERE pid='%1'").arg(m_currentPlaylist);
    QStringList names = m_handler->query(m_handler->m_libraryDb, sql);
    m_playlistName = names[0];

    debug() << "playlist name: " << m_playlistName;
}

void
IphoneOsPlaylistCapability::nextPlaylistToParse()
{
    ++m_currentPlaylistIndex;
}

bool
IphoneOsPlaylistCapability::shouldNotParseNextPlaylist()
{
    return false;
}

void
IphoneOsPlaylistCapability::prepareToParsePlaylistTracks()
{
    QString sql = QString("SELECT item_pid FROM item_to_container WHERE container_pid='%1'").arg(m_currentPlaylist);
    m_pids = m_handler->query(m_handler->m_libraryDb, sql);
    m_currentPidIndex = 0;
    debug() << "playlist pids: " << m_playlistName << " " << m_pids;

    prepareToParseNextPlaylistTrack();
}

bool
IphoneOsPlaylistCapability::isEndOfParsePlaylist()
{
    return m_currentPidIndex >= m_pids.size()-1;
}

void
IphoneOsPlaylistCapability::prepareToParseNextPlaylistTrack()
{
    m_currentPid = m_currentPidIndex < m_pids.size() ?  m_pids[m_currentPidIndex] : QString();
    debug() << "next track: " << m_currentPid << m_currentPidIndex;
}

void
IphoneOsPlaylistCapability::nextPlaylistTrackToParse()
{
    ++m_currentPidIndex;
    prepareToParseNextPlaylistTrack();
}

Meta::MediaDeviceTrackPtr
IphoneOsPlaylistCapability::libGetTrackPtrForTrackStruct()
{
    //return m_handler->metaForPid(m_currentPid);
    // the data to be returned has to be available in the collection
    return m_handler->m_trackhash.key( m_currentPid );
}

QString
IphoneOsPlaylistCapability::libGetPlaylistName()
{
    return m_playlistName;
}

void
IphoneOsPlaylistCapability::savePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( playlist )
    Q_UNUSED( name )
}

void
IphoneOsPlaylistCapability::deletePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( playlist )
}

void
IphoneOsPlaylistCapability::renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( playlist )
}


#include "IphoneOsPlaylistCapability.moc"
