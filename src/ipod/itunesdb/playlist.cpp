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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "playlist.h"
#include <qbuffer.h>

namespace itunesdb {

Playlist::Playlist()
    : ListItem(ITEMTYPE_PLAYLIST)
{
}

Playlist::~Playlist() {
    tracklist.clear();
}

const QString& Playlist::getTitle() const
{
    return getItemProperty( MHOD_TITLE);
}


void Playlist::doneAddingData() {
    if( getItemProperty(MHOD_TITLE).isEmpty())
        setItemProperty("_no_title_", MHOD_TITLE);
}

/*!
    \fn Playlist::setTitle( QString& newtitle)
 */
void Playlist::setTitle( const QString& newtitle)
{
    setItemProperty( newtitle, MHOD_TITLE);
    doneAddingData();    // consisteny check
}

uint Playlist::addPlaylistItem(const PlaylistItem& item) {
    return addPlaylistItem(item.getID());
}

/*!
    \fn Playlist::addTrack( Q_UINT32& trackid)
 */
uint Playlist::addPlaylistItem(const Q_UINT32& trackid)
{
    tracklist.append(trackid);
    return tracklist.count() - 1;
}


Q_UINT32 Playlist::removeTrackAt( Iterator& pos) {
    Q_UINT32 retval= *(--pos._iterator);
    if( pos._iterator != pos._list.end())
        pos._iterator= tracklist.erase( pos._iterator);
    return retval;
}


Q_UINT32 Playlist::setTrackIDAt( uint pos, Q_UINT32 newtrackid) {
    Q_UINT32 retval;
    if( pos < tracklist.count()) {
        retval= tracklist[pos];
        tracklist[pos]= newtrackid;
    } else {
        retval= TRACKLIST_UNDEFINED;
    }
    
    return retval;
}


Playlist::Iterator Playlist::getTrackIDs() {
    return Iterator( tracklist);
}

/*!
    \fn itunesdb::Playlist::getNumTracks()
 */
uint Playlist::getNumTracks() const
{
    return tracklist.count();
}


void Playlist::clear() {
    tracklist.clear();
    setItemProperty(QString::null, MHOD_TITLE);
}

/*!
    \fn itunesdb::Playlist::writePlaylistData( QByteArray& data)
 */
void Playlist::writeData( QByteArray& data, bool isMainlist)
{
    QBuffer buffer( data);
    buffer.open(IO_WriteOnly);
    QDataStream stream( &buffer);
    stream.setByteOrder( QDataStream::LittleEndian);
    
    /** Write the header **/
    
    stream << (Q_UINT32) 0x7079686D;    // mhyp
    stream << (Q_UINT32) 0x6C;    // headerlen
    stream << (Q_UINT32) 0x0;    // length - set later
    
    stream << (Q_UINT32) 2;    // 2 mhods (const)
    stream << (Q_UINT32) getNumTracks();
    stream << (Q_UINT32) (isMainlist ? 1 : 0);
    for( int i= 0; i< 21; i++)
        stream << (Q_UINT32) 0;    // pad the rest

    for( int i= 0; i< 2; i++) {
        const char *data= (const char *)getTitle().ucs2();
        if( data == NULL)
            continue;
        
        int datalen= 2* getTitle().length();
        
        stream << (Q_UINT32) 0x646F686D;    // mhod
        stream << (Q_UINT32) 0x18;
        stream << (Q_UINT32) 40+ datalen;
        stream << (Q_UINT32) MHOD_TITLE;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 1;    // dummy - would refer to the trackID if used in playlist
        stream << (Q_UINT32) datalen;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 0;
        stream.writeRawBytes( data, datalen);
    }
    
    // write playlist items
    // TODO this line causes non const of this function, 'cause tracklist::Iterator is non const (mutable?)
    Iterator trackiterator= getTrackIDs();
    while( trackiterator.hasNext()) {
        Q_UINT32 trackid= trackiterator.next();
        
        stream << (Q_UINT32) 0x7069686D;    // mhip
        stream << (Q_UINT32) 0x4C;    // headerlen
        stream << (Q_UINT32) 0x4C;    // datalen
        stream << (Q_UINT32) 1;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) trackid;
        stream << (Q_UINT32) trackid;
        for( int i= 0; i<12; i++)
            stream << (Q_UINT32) 0;
        
        stream << (Q_UINT32) 0x646F686D;
        stream << (Q_UINT32) 0x18;
        stream << (Q_UINT32) 0x2C;
        stream << (Q_UINT32) 0x64;    // type: playlist
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) trackid;
        for( int i= 0; i< 4; i++)
            stream << (Q_UINT32) 0;
    }
        
    buffer.at( 8);
    stream << (Q_UINT32)data.size();
    buffer.close();
}


QDataStream & Playlist::writeToStream(QDataStream & outstream, bool isMainlist) {
    QByteArray buffer;
    writeData(buffer, isMainlist);
    outstream.writeRawBytes( buffer.data(), buffer.size());
    
    return outstream;
}

};
