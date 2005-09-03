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
#ifndef ITUNESDBPLAYLIST_H
#define ITUNESDBPLAYLIST_H

#include <qvaluevector.h>

#include "listitem.h"
#include "ipod_playlistitem.h"

#define TRACKLIST_UNDEFINED 0xFFFFFFFF

namespace itunesdb {

/**
 @author Michael Schulze
*/
class IPodPlaylist : public ListItem
{
public:
    typedef QValueVector<Q_UINT32> TrackList_T;
    class Iterator {
    protected:
        TrackList_T& _list;
        TrackList_T::iterator _iterator;
        friend class itunesdb::IPodPlaylist;
    public:
        Iterator(TrackList_T& list)
            : _list(list) {
            _iterator = _list.begin();
        }
        bool hasNext() {
            return _iterator != _list.end();
        }
        const Q_UINT32& next() {
            return *_iterator++;
        }
    };

    IPodPlaylist();
    virtual ~IPodPlaylist();

    const QString& getTitle() const;
    void setTitle( const QString& newtitle);
    void doneAddingData();

    virtual uint addPlaylistItem(const IPodPlaylistItem& item);
    virtual uint addPlaylistItem(const Q_UINT32& trackid);
    virtual Q_UINT32 removeTrackAt( Iterator& pos);
    virtual Q_UINT32 setTrackIDAt( uint pos, Q_UINT32 trackid);

    Iterator getTrackIDs();
    uint getNumTracks() const;

    void clear();

    void writeData( QByteArray& data, bool isMainlist);
    QDataStream & writeToStream (QDataStream & outstream, bool isMainlist);

protected:
    void writeHeader( QDataStream& stream, bool isMainlist );
    void writeTitle( QDataStream& stream );
    void writeLongPlaylist( QDataStream& stream );
    void writeTracks( QDataStream& stream );

    TrackList_T tracklist;
    //bool isLibraryList;         // PL_TYPE_MPL: master play list
    bool isSmartList;      // smart playlist?
    Q_INT32 timeStamp;     // some timestamp
    Q_INT64 id;            // playlist ID
    Q_INT32 unk3;
    Q_INT32 sCount;
    Q_INT32 lCount;
    //SPLPref splpref;      /* smart playlist prefs          */
    //SPLRules splrules;    /* rules for smart playlists     */
};

}

#endif
