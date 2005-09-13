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
#include "itunesdbparser.h"
#include <qdatastream.h>

namespace itunesdb {

ItunesDBParser::ItunesDBParser( ItunesDBListener& ituneslistener)
{
    listener= &ituneslistener;
}


ItunesDBParser::~ItunesDBParser()
{
}


void ItunesDBParser::seekRelative(QDataStream& stream, uint numbytes)
{
    if ( numbytes ) {
        char* buffer = new char[ numbytes ];
        stream.readRawBytes(buffer, numbytes);
        delete [] buffer;
    }
}

/*!
    \fn itunesdb::ItunesDBParser::parse(QFile& file)
 */
void ItunesDBParser::parse(QFile& file)
{
    Q_UINT32 magic, headerlen, filesize;

    IPodPlaylistItem current_playlistitem;
    Track current_track;

    ListItem * listitem = NULL;

    listener->parseStarted();

    if( !file.exists() || !file.open( IO_ReadOnly)) {
        listener->handleError( file.name()+ " could not be opened!");
        return;    // no need to cleanup
    }

    QDataStream stream( &file);
    stream.setByteOrder( QDataStream::LittleEndian);

    stream >> magic;
    if( magic != 0x6462686D) {
        listener->handleError( file.name() + " is not an itunesDB file");
        goto ItunesDBParser_parse_cleanup;
    }

    stream >> headerlen;
    stream >> filesize;

    // skip the rest of the header
    seekRelative(stream, headerlen- 12);

    // begin with actual parsing
    while( !stream.atEnd()) {
        Q_UINT32 blocktype;
        Q_UINT32 blocklen;

        stream >> blocktype;

        switch( blocktype) {
        case 0x6473686D:    // mhsd - yeah, good to know
            stream >> blocklen;
            // iTunesDB has two large blocks - one with the tracks and one with playlists
            // both begin with this tag. Since the data entrys themself have different block
            // types we can safely ignore this information here ...
            if( listitem != NULL) {    // OK, so now we're at the end of the first mshd
                handleItem( *listitem);
                listitem = NULL;
            }
            seekRelative(stream, blocklen- 8);
            break;

        case 0x746C686D: {    // mhlt: number of tracks (and other metainfo?)
            Q_UINT32 numtracks;
            stream >> blocklen;
            if( listitem != NULL) {
                handleItem( *listitem);
                listitem = NULL;
            }
            stream >> numtracks;
            listener->setNumTracks( numtracks);
            seekRelative(stream, blocklen- 12);    // skip the rest
            }
            break;

        case 0x7469686D: {    // mhit - start of a track
            if( listitem != NULL) {
                handleItem( *listitem);
            }

            current_track = Track();
            current_track.readFromStream(stream);
            listitem = &current_track;
            }
            break;

        case 0x646F686D: {    // mhod
            Q_UINT32 type;
            stream >> blocklen;
            stream >> blocklen;

            uint ucs2len = ( blocklen - 40 ) / sizeof( Q_UINT16 );

            stream >> type;

            if( type == itunesdb::MHOD_PLAYLIST)
                break;    // ignore

            unsigned short* buffer = new unsigned short[ucs2len+1];

            seekRelative(stream, 24);    // skip stuff
            for ( int i = 0; i < ucs2len; ++i ) {
              Q_UINT16 h;
              stream >> h;
              buffer[i] = h;
            }

            buffer[ucs2len]= 0;

            if( listitem != NULL)
                listitem->setItemProperty( QString::fromUcs2(buffer), (ItemProperty)type);

            delete [] buffer;
            }
            break;
        case 0x706C686D: {    // mhlp
            Q_UINT32 numplaylists;
            if( listitem != NULL) {
                handleItem( *listitem);
                listitem = NULL;
            }
            stream >> blocklen;
            stream >> numplaylists;
            listener->setNumPlaylists( numplaylists);
            seekRelative(stream, blocklen- 12);
            }
            break;

        case 0x7079686D: {    // mhyp: Playlist
            stream >> blocklen;
            if( listitem != NULL) {
                handleItem( *listitem);
            }
            if( !current_playlist.getTitle().isEmpty()) {
                // there's a playlist waiting to be handed over to the listener
                listener->handlePlaylist( current_playlist);
            }
            current_playlist = IPodPlaylist();
            listitem = &current_playlist;    // TODO working with a copy is wrong here
            seekRelative(stream, blocklen- 8);
            }
            break;
        case 0x7069686D: {    // mhip: PlaylistItem
            Q_UINT32 itemid;

            stream >> blocklen;
            if( listitem != NULL) {
                handleItem( *listitem);
            }

            seekRelative(stream, 16);
            stream >> itemid;

            current_playlistitem = IPodPlaylistItem( itemid);
            listitem = &current_playlistitem;

            seekRelative(stream, blocklen- 28);
            }
            break;
        default:
            listener->handleError( QString( "unknown tag found! stop parsing"));
            goto ItunesDBParser_parse_cleanup;
        }
    }

    if( listitem != NULL) {
        handleItem( *listitem);
        listitem = NULL;
    }
    if( !current_playlist.getTitle().isEmpty()) {
        // there's a playlist waiting to be handed over to the listener
        listener->handlePlaylist( current_playlist);
    }
    listener->parseFinished();

ItunesDBParser_parse_cleanup:    // !!! only cleanup code from here on !!!
    file.close();
}

}


/*!
    \fn itunesdb::ItunesDBParser::handleItem( ListItem& item)
 */
void itunesdb::ItunesDBParser::handleItem( ListItem& item)
{
    switch( item.getType()) {
    case ITEMTYPE_TRACK: {
        Track * pTrack = dynamic_cast <Track *> (&item);
        if(pTrack == NULL || pTrack->getID() == 0)
            break;
        pTrack->doneAddingData();
        listener->handleTrack( *pTrack);
        }
        break;
    case ITEMTYPE_PLAYLIST: {
        IPodPlaylist * pPlaylist = dynamic_cast <IPodPlaylist *> (&item);
        if( pPlaylist == NULL)
            break;
        pPlaylist->doneAddingData();
        // listener->addPlaylist( *pPlaylist);
        }
        break;
    case ITEMTYPE_PLAYLISTITEM: {
        IPodPlaylistItem * pPlaylistitem = dynamic_cast <IPodPlaylistItem *> (&item);
        if( pPlaylistitem == NULL)
            break;
        pPlaylistitem->doneAddingData();
        current_playlist.addPlaylistItem( *pPlaylistitem);
        }
        break;
    }
}

