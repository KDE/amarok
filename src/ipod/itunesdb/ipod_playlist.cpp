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
#include "ipod_playlist.h"
#include <qbuffer.h>

namespace itunesdb {

IPodPlaylist::IPodPlaylist()
: ListItem(ITEMTYPE_PLAYLIST),
isSmartList(false), 
timeStamp(0),
id(0)
{
}

IPodPlaylist::~IPodPlaylist() 
{
    tracklist.clear();
}

const QString& IPodPlaylist::getTitle() const
{
    return getItemProperty( MHOD_TITLE);
}


void IPodPlaylist::doneAddingData() {
    if( getItemProperty(MHOD_TITLE).isEmpty())
        setItemProperty("_no_title_", MHOD_TITLE);
}

/*!
    \fn IPodPlaylist::setTitle( QString& newtitle)
 */
void IPodPlaylist::setTitle( const QString& newtitle)
{
    setItemProperty( newtitle, MHOD_TITLE);
    doneAddingData();    // consisteny check
}

uint IPodPlaylist::addPlaylistItem(const IPodPlaylistItem& item) {
    return addPlaylistItem(item.getID());
}

/*!
    \fn IPodPlaylist::addTrack( Q_UINT32& trackid)
 */
uint IPodPlaylist::addPlaylistItem(const Q_UINT32& trackid)
{
    tracklist.append(trackid);
    return tracklist.count() - 1;
}


Q_UINT32 IPodPlaylist::removeTrackAt( Iterator& pos) {
    Q_UINT32 retval= *(--pos._iterator);
    if( pos._iterator != pos._list.end())
        pos._iterator= tracklist.erase( pos._iterator);
    return retval;
}


Q_UINT32 IPodPlaylist::setTrackIDAt( uint pos, Q_UINT32 newtrackid) {
    Q_UINT32 retval;
    if( pos < tracklist.count()) {
        retval= tracklist[pos];
        tracklist[pos]= newtrackid;
    } else {
        retval= TRACKLIST_UNDEFINED;
    }

    return retval;
}


IPodPlaylist::Iterator IPodPlaylist::getTrackIDs() {
    return Iterator( tracklist);
}

/*!
    \fn itunesdb::IPodPlaylist::getNumTracks()
 */
uint IPodPlaylist::getNumTracks() const
{
    return tracklist.count();
}


void IPodPlaylist::clear() {
    tracklist.clear();
    setItemProperty(QString::null, MHOD_TITLE);
}


/**
Writes the header
*/
void IPodPlaylist::writeHeader( QDataStream& stream, bool isMainlist )
{
    stream << (Q_UINT32) 0x7079686D;    // mhyp
    stream << (Q_UINT32) 0x6C;    // headerlen
    stream << (Q_UINT32) 0x0;    // length - set later
    stream << (Q_UINT32) (isSmartList ? 4 : 2 );    // 2 mhods (const), smartlist have 4
    stream << (Q_UINT32) getNumTracks();
    stream << (Q_UINT32) (isMainlist ? 1 : 0);  // 1=hidden (library lis), 2=visible
    stream << (Q_UINT32) timeStamp;
    stream << (Q_UINT64) id;
    stream << (Q_UINT32) 0;     // unknown, but significant
    stream << (Q_UINT32) 0;     // number of string MHODs
    stream << (Q_UINT32) 0;     // The number of Type 52 MHODs
    for( int i= 0; i< 15; i++)
        stream << (Q_UINT32) 0;    // pad the rest
}

/*!
    \fn itunesdb::IPodPlaylist::writePlaylistData( QByteArray& data)
 */
void IPodPlaylist::writeTitle( QDataStream& stream )
{
    QString title = getTitle();
    if ( title.isEmpty() )
        title = "amaroK";
    const char *data = (const char *)title.ucs2();
    if( data == NULL)
        return;

    int datalen= 2* title.length();
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

/*!
    \fn itunesdb::IPodPlaylist::writePlaylistData( QByteArray& data)
 */
void IPodPlaylist::writeLongPlaylist( QDataStream& stream )
{
    stream << (Q_UINT32) 0x646F686D;    // mhod
    stream << (Q_UINT32) 0x18;          // size of header
    stream << (Q_UINT32) 0x0288;        // size of header + body
    stream << (Q_UINT32) MHOD_PLAYLIST; // type of the entry
    for( int i= 0; i< 6; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x010084;      // unknown
    stream << (Q_UINT32) 0x05;              // unknown
    stream << (Q_UINT32) 0x09;              // unknown
    stream << (Q_UINT32) 0x03;              // unknown
    stream << (Q_UINT32) 0x120001;          // unknown
    for( int i= 0; i< 3; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0xc80002;          // unknown
    for( int i= 0; i< 3; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x3c000d;          // unknown
    for( int i= 0; i< 3; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x7d0004;          // unknown
    for( int i= 0; i< 3; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x7d0003;          // unknown
    for( int i= 0; i< 3; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x640008;          // unknown
    for( int i= 0; i< 3; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x640017;          // unknown
    stream << (Q_UINT32) 0x01;              // bool? (visible? / colums?)
    for( int i= 0; i< 2; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x500014;          // unknown
    stream << (Q_UINT32) 0x01;              // bool? (visible?)
    for( int i= 0; i< 2; i++)
        stream << (Q_UINT32) 0;
    stream << (Q_UINT32) 0x7d0015;          // unknown
    stream << (Q_UINT32) 0x01;              // bool? (visible?)
    for( int i= 0; i< 114; i++)
        stream << (Q_UINT32) 0;
}

/**
Writes the track list
*/
void IPodPlaylist::writeTracks( QDataStream& stream )
{
    // write playlist items
    // TODO this line causes non const of this function, 'cause tracklist::Iterator is non const (mutable?)
    Iterator trackiterator= getTrackIDs();
    int pos = 0;
    while( trackiterator.hasNext()) 
    {
        Q_UINT32 trackid= trackiterator.next();

        stream << (Q_UINT32) 0x7069686D;    // mhip
        stream << (Q_UINT32) 0x4C;          // headerlen
        stream << (Q_UINT32) 0x4C;          // datalen
        stream << (Q_UINT32) 1;
        stream << (Q_UINT32) pos;
        stream << (Q_UINT32) 0;             // uknown but significant
        stream << (Q_UINT32) trackid;
        for( int i= 0; i<12; i++)
            stream << (Q_UINT32) 0;

        stream << (Q_UINT32) 0x646F686D;   // mhod
        stream << (Q_UINT32) 0x18;
        stream << (Q_UINT32) 0x2C;
        stream << (Q_UINT32) MHOD_PLAYLIST;    // type: playlist
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) pos;
        for( int i= 0; i< 4; i++)
            stream << (Q_UINT32) 0;
        pos++;
    }
}

/*!
    \fn itunesdb::IPodPlaylist::writePlaylistData( QByteArray& data)
 */
void IPodPlaylist::writeData( QByteArray& data, bool isMainlist)
{
    QBuffer buffer( data);
    buffer.open(IO_WriteOnly);
    QDataStream stream( &buffer);
    stream.setByteOrder( QDataStream::LittleEndian);

    writeHeader( stream, isMainlist );
    writeTitle( stream );
    writeLongPlaylist( stream );

    if ( isSmartList ) // is optional
    {  /* write the smart rules */
        //mk_mhod (cts, MHOD_ID_SPLPREF, &pl->splpref);
        //mk_mhod (cts, MHOD_ID_SPLRULES, &pl->splrules);
    }

    writeTracks( stream );

    buffer.at( 8);
    stream << (Q_UINT32)data.size();
    buffer.close();
}

/**
*/
QDataStream & IPodPlaylist::writeToStream(QDataStream & outstream, bool isMainlist) {
    QByteArray buffer;
    writeData(buffer, isMainlist);
    outstream.writeRawBytes( buffer.data(), buffer.size());

    return outstream;
}

}
