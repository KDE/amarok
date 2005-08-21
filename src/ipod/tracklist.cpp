/***************************************************************************
 *   Copyright (C) 2004 by Michael Schulze                                 *
 *   mike.s@genion.de                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "tracklist.h"

TrackList::TrackList()
    : IPodPlaylist(), max_tracknumber(0), unsaved_changes(false)
{
}

TrackList::TrackList(const IPodPlaylist& playlist)
    : IPodPlaylist(playlist), max_tracknumber(0), unsaved_changes(false)
{
    max_tracknumber = playlist.getNumTracks();
}


TrackList::~TrackList()
{
}


uint TrackList::addPlaylistItem(const TrackMetadata& track)
{
    if(max_tracknumber < track.getTrackNumber())
        max_tracknumber = track.getTrackNumber();

    return addPlaylistItem(track.getID());
}

uint TrackList::addPlaylistItem(const Q_UINT32& trackid)
{
    unsaved_changes = true;
    return IPodPlaylist::addPlaylistItem(trackid);
}


void TrackList::removeAll(Q_UINT32 trackid) {
    // TODO check if maxtrackid and maxtracknumber is still valid
    Iterator trackiter = getTrackIDs();
    while(trackiter.hasNext()) {
        if(trackiter.next() == trackid) {
            removeTrackAt(trackiter);
        }
    }
}

Q_UINT32 TrackList::removeTrackAt(Iterator& pos) {
    // TODO check if maxtrackid and maxtracknumber is still valid
    unsaved_changes = true;
    return IPodPlaylist::removeTrackAt(pos);
}


Q_UINT32 TrackList::setTrackIDAt( uint pos, Q_UINT32 newtrackid) {
    // TODO check if maxtrackid and maxtracknumber is still valid
    unsaved_changes = true;
    return IPodPlaylist::setTrackIDAt(pos, newtrackid);
}


Q_UINT32 TrackList::getMaxTrackNumber() const {
    if (max_tracknumber != 0)
        return max_tracknumber;
    else
        return getNumTracks();    // fallback
}


/*!
    \fn TrackList::setTitle( QString& newtitle)
 */
void TrackList::setTitle( const QString& newtitle)
{
    IPodPlaylist::setTitle(newtitle);
    doneAddingData();    // consisteny check
    unsaved_changes = true;
}

