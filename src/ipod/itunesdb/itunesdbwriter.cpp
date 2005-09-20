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
#include "itunesdbwriter.h"

#include <qcstring.h>
#include <qdatastream.h>
#include <qbuffer.h>

#define MHSD_HEADERLEN 0x60

namespace itunesdb {

ItunesDBWriter::ItunesDBWriter(ItunesDBDataSource* trackdatasource)
{
    datasource= trackdatasource;
}


ItunesDBWriter::~ItunesDBWriter()
{
}


}


/*!
    \fn itunesdb::ItunesDBWriter::write(QFile& file)
 */
void itunesdb::ItunesDBWriter::write(QFile& file)
{
    QByteArray trackdata;
    QByteArray playlistdata;

    // TODO do more consistency checks with the data

    if( datasource->getMainplaylist() == NULL) {
        datasource->handleError("Main Tracklist could not be found!");
        return;    // ick
    }

    if( !file.open( IO_WriteOnly)) {
        datasource->handleError(file.name()+ " could not be opened for writing!");
        return;
    }

    datasource->writeInit();

    fillTrackBuffer( trackdata);
    fillPlaylistBuffer( playlistdata);

    // write to file
    QDataStream stream( &file);
    stream.setByteOrder( QDataStream::LittleEndian);

    stream << (Q_UINT32) 0x6462686D;    // write magic = mhbd
    stream << (Q_UINT32) 0x68;    // headerlen = 104 byte
    stream << (Q_UINT32) 0x0;    // length of the whole file - set at the end
    stream << (Q_UINT32) 0x1;
    stream << (Q_UINT32) 0x0d;    // iTunes version 4.9
    stream << (Q_UINT32) 0x2;   // iTunes 4.x has 2 MHSD children (4.9 has can have a third for podcasts )
    stream << (Q_UINT64) 0x0; // unkown 64 bit 
    stream << (Q_UINT32) 0x2;
    for( int i= 0; i< 17; i++)
        stream << (Q_UINT32) 0;

    // write track data
    stream.writeRawBytes( trackdata.data(), trackdata.size());

    // write playlist data
    stream.writeRawBytes( playlistdata.data(), playlistdata.size());

    file.at( 8);
    stream << (Q_UINT32) file.size();

    file.close();
    datasource->writeFinished();
}


/*!
    \fn itunesdb::ItunesDBWriter::writeSD(QFile& file)
 */
void itunesdb::ItunesDBWriter::writeSD( QFile& file )
{
    if( datasource->getMainplaylist() == NULL) {
        datasource->handleError("Main Tracklist could not be found!");
        return;    // ick
    }

    if( !file.open( IO_WriteOnly)) {
        datasource->handleError(file.name()+ " could not be opened for writing!");
        return;
    }

    // write to file
    QDataStream stream( &file );
    stream.setByteOrder( QDataStream::BigEndian );

    //stream << (Q_UINT8) 0 << (Q_UINT16) datasource->getNumTracks(); // limited to 64k songs
    //stream << (Q_UINT8) 0x01 << (Q_UINT16) 0x0600;
    //stream << (Q_UINT8) 0x00 << (Q_UINT16) 0x0012;
    write3ByteLittle( stream, datasource->getNumTracks() );
    write3ByteLittle( stream, 0x010600 );
    write3ByteLittle( stream, 0x000012 );
    for( int i= 0; i< 9; i++)
        stream << (Q_UINT8) 0;

    QByteArray trackdata;
    fillTrackBufferSD( trackdata );
    // write track data
    stream.writeRawBytes( trackdata.data(), trackdata.size());

    file.close();
}


/*!
    \fn itunesdb::ItunesDBWriter::writeTracks( QDataStream& stream)
 */
void itunesdb::ItunesDBWriter::fillTrackBuffer( QByteArray& buffer)
{
    QBuffer io_buffer( buffer);
    io_buffer.open(IO_WriteOnly);
    QDataStream stream( &io_buffer);
    stream.setByteOrder( QDataStream::LittleEndian);

    // write mhsd
    stream << (Q_UINT32) 0x6473686D;    // "mhsd"
    stream << (Q_UINT32) MHSD_HEADERLEN;    // headerlen
    stream << (Q_UINT32) 0x0;    // length - set when we're done
    stream << (Q_UINT32) 1;        // type: tracklist
    for( int i= 0; i< 20; i++)
        stream << (Q_UINT32) 0;    // pad the rest (80 bytes)

    // tracklist header
    stream << (Q_UINT32) 0x746C686D;    // mhlt
    stream << (Q_UINT32) 0x5C;    // headerlen
    stream << (Q_UINT32) datasource->getNumTracks();    // number of tracks
    for( int i= 0; i< 20; i++)
        stream << (Q_UINT32) 0;    // pad the rest (80 bytes)

    // list the tracks
    for( Track * track= datasource->firstTrack(); track != NULL; track= datasource->nextTrack()) {
        track->writeToStream(stream);
    }

    io_buffer.at( 8);
    stream << (Q_UINT32)io_buffer.size();    // write the length

    io_buffer.close();
}


/**
 *
void itunesdb::ItunesDBWriter::fillPodcastBuffer( QByteArray& buffer )
{
    QBuffer io_buffer( buffer);
    io_buffer.open(IO_WriteOnly);
    QDataStream stream( &io_buffer);
    stream.setByteOrder( QDataStream::LittleEndian);

    stream << (Q_UINT32) 0x6473686D;    // "mhsd"
    stream << (Q_UINT32) MHSD_HEADERLEN;    // headerlen
    stream << (Q_UINT32) 0x0;    // length - set when we're done
    stream << (Q_UINT32) 3;        // type: podcastlist
    for( int i= 0; i< 20; i++)
        stream << (Q_UINT32) 0;    // pad the rest (80 bytes)
    
    stream << (Q_UINT32) 0x706C686D;     // "mhlp"
    stream << (Q_UINT32) 0x5C;    // headerlen
    stream << (Q_UINT32) 1;          // one podlist
    for( int i= 0; i< 20; i++)
        stream << (Q_UINT32) 0;    // pad the rest (80 bytes)
    // ???
    
    io_buffer.at( 8);
    stream << (Q_UINT32)io_buffer.size();    // write the length
    
    io_buffer.close();
}*/


/*!
    \fn itunesdb::ItunesDBWriter::fillPlaylistBuffer( QByteArray& buffer)
 */
void itunesdb::ItunesDBWriter::fillPlaylistBuffer( QByteArray& buffer)
{
    QBuffer io_buffer( buffer);
    io_buffer.open(IO_WriteOnly);
    QDataStream stream( &io_buffer);
    stream.setByteOrder( QDataStream::LittleEndian);

    IPodPlaylist * mainlist= datasource->getMainplaylist();
    if(mainlist == NULL)
        return;

    // write mhsd
    stream << (Q_UINT32) 0x6473686D;    // "mhsd"
    stream << (Q_UINT32) MHSD_HEADERLEN;    // headerlen
    stream << (Q_UINT32) 0x0;    // length - set when we're done
    stream << (Q_UINT32) 2;        // type: playlist
    for( int i= 0; i< 20; i++)
        stream << (Q_UINT32) 0;    // pad the rest (80 bytes)

    // write mhlp
    stream << (Q_UINT32) 0x706C686D;    // "mhlp"
    stream << (Q_UINT32) 0x5C;    // headerlen
    stream << (Q_UINT32) datasource->getNumPlaylists()+ 1;    // number of playlists incl. mainlist
    for( int i= 0; i< 20; i++)
        stream << (Q_UINT32) 0;    // pad the rest (80 bytes)

    // write the mainlist
    mainlist->writeToStream(stream, true);

    for( IPodPlaylist * playlist= datasource->firstPlaylist(); playlist != NULL; playlist= datasource->nextPlaylist()) {
        playlist->writeToStream(stream, false);
    }

    io_buffer.at( 8);
    stream << (Q_UINT32)io_buffer.size();    // write the length

    io_buffer.close();
}


/*!
    \fn itunesdb::ItunesDBWriter::writeTracks( QDataStream& stream)
 */
void itunesdb::ItunesDBWriter::fillTrackBufferSD( QByteArray& buffer)
{
    QBuffer io_buffer( buffer);
    io_buffer.open(IO_WriteOnly);
    QDataStream stream( &io_buffer);
    stream.setByteOrder( QDataStream::BigEndian);

    // list the tracks
    for( Track * track= datasource->firstTrack(); track != NULL; track= datasource->nextTrack()) 
    {
        //track->writeToStream(stream);
        write3ByteLittle( stream, 0x22e );
        write3ByteLittle( stream, 0x5aa501 );
        write3ByteLittle( stream, 0 ); // starttime
        write3ByteLittle( stream, 0 );
        write3ByteLittle( stream, 0 );
        write3ByteLittle( stream, 0 ); // stoptime
        write3ByteLittle( stream, 0 );
        write3ByteLittle( stream, 0 );
        write3ByteLittle( stream, 0x64 ); // volume=0
        write3ByteLittle( stream, 0x01 ); // FIXME only MP3 currently
        write3ByteLittle( stream, 0x200 );

        // filename
        QString filename = track->getItemProperty( MHOD_PATH );
        // make "/" out of ":"
        int idx = filename.find( ':' );
        while ( idx>-1 )
        {
            filename[idx] = '/';
            idx = filename.find( ':' );
        }
        
        const char *data= (const char *)filename.ucs2();
        int datalen = 0;
        if( data != NULL)
        {
            datalen = 2* filename.length();
            stream.writeRawBytes( data, datalen );
        }
        int padlength = 522 - datalen;
        for ( int i=0; i<padlength; i++ )
            stream << (Q_UINT8) 0;

        stream << (Q_UINT8) 0x01;
        stream << (Q_UINT8) 0;
        stream << (Q_UINT8) 0;
    }
    io_buffer.close();
}


/**
 */
void itunesdb::ItunesDBWriter::write3ByteLittle( QDataStream &stream, int v )
{
    stream << (Q_UINT8) ((v>>16) & 0xff);
    stream << (Q_UINT8) ((v>>8) & 0xff);
    stream << (Q_UINT8) (v & 0xff);
}

