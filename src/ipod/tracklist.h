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
#ifndef TRACKLIST_H
#define TRACKLIST_H

#include "itunesdb/ipod_playlist.h"
#include "trackmetadata.h"

/**
Container for a playlist or an album

@author Michael Schulze
*/
class TrackList : public itunesdb::IPodPlaylist
{
public:
    TrackList();
    TrackList(const IPodPlaylist& playlist);
    ~TrackList();

    uint addPlaylistItem(const TrackMetadata& track);

    Q_UINT32 removeTrackAt(Iterator& pos);
    Q_UINT32 setTrackIDAt( uint pos, Q_UINT32 newtrackid);
    void removeAll(Q_UINT32 trackid);

    void setTitle( const QString& newtitle);

    Q_UINT32 getMaxTrackNumber() const;
    void setMaxTrackNumber(Q_UINT32 max_tracknum) { max_tracknumber = max_tracknum; }
    bool unsavedChanges() const { return unsaved_changes; }
    void setChangeFlag(bool state) { unsaved_changes = state; }

private:
    uint addPlaylistItem(const Q_UINT32& trackid);
    Q_UINT32 max_tracknumber;
    bool unsaved_changes;
};

#endif
