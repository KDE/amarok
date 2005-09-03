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
#include "track.h"
#include <qdatastream.h>
#include <qbuffer.h>
#include <qdatetime.h>

#define MAC_EPOCH_DELTA 2082844800

namespace itunesdb {

Track::Track()
    : ListItem( ITEMTYPE_TRACK)
{
    rating         = 0;
    compilation    = 0;
    vbr            = 0;
    type           = 0;
    lastmodified   = 0;
    file_size      = 0;
    tracklen       = 0;
    tracknum       = 0;
    numtracks      = 0;
    year           = 0;
    bitrate        = 0;
    samplerate     = 0;
    volumeadjust   = 0;
    playcount      = 0;
    last_played_at = 0;
    cdnum          = 0;
    numcds         = 0;
    id             = 0;
    date_added     = QDateTime::currentDateTime().toTime_t()+ MAC_EPOCH_DELTA;
    file_format_code = 0xC;
    dbid = 0;
}


Track::~Track() {
}


void Track::writeData( QByteArray& data) const {
    QBuffer buffer( data);
    buffer.open(IO_WriteOnly);
    QDataStream stream( &buffer);
    stream.setByteOrder( QDataStream::LittleEndian);

    /** Write the track header **/
    stream << (Q_UINT32) 0x7469686D;        // 0x00 mhit
    stream << (Q_UINT32) 0xf4;              // 0x04 headerlen
    stream << (Q_UINT32) 0x0;               // 0x08 length - set later
    stream << (Q_UINT32) 0x0;               // 0x0C number of mhods
    stream << (Q_UINT32) getID();           // 0x10
    stream << (Q_UINT32) 1;                 // 0x14
    //stream << (Q_UINT32) 0;                 // 0x18
    stream << (Q_UINT32) 0x4d503320;        // ipod shiffle wants a "MP3 " here
    stream << vbr;                          // 0x1C
    stream << type;                         // 0x1D
    stream << compilation;                  // 0x1E
    stream << rating;                       // 0x1F
    stream << (Q_UINT32) getLastModified()+ MAC_EPOCH_DELTA; // 0x20
    stream << (Q_UINT32) getFileSize();     // 0x24
    stream << (Q_UINT32) getTrackLength();  // 0x28
    stream << (Q_UINT32) getTrackNumber();  // 0x2C
    stream << (Q_UINT32) getTrackCount();   // 0x30
    stream << (Q_UINT32) getYear();         // 0x34
    stream << (Q_UINT32) getBitrate();      // 0x38
    stream << (Q_UINT32) getSamplerate();   // 0x3C
    stream << (Q_UINT32) getVolumeAdjust(); // 0x40
    stream << (Q_UINT32) 0;                 // 0x44 empty space
    //stream << (Q_UINT32) getTrackLength();  // 0x48 empty space
    stream << (Q_UINT32) 0;  // 0x48 empty space
    stream << (Q_UINT32) 0;                 // 0x4C empty space
    stream << (Q_UINT32) getPlayCount();    // 0x50
    stream << (Q_UINT32) getPlayCount();    // 0x54
    stream << (Q_UINT32) getLastPlayed();   // 0x58
    stream << (Q_UINT32) getCdNumber();     // 0x5C
    stream << (Q_UINT32) getCdCount();      // 0x60
    stream << (Q_UINT32) 0;                 // 0x64 empty space //userid from apple store
    stream << (Q_UINT32) date_added;        // 0x68
    stream << (Q_UINT32) 0;                 // boockmarktime
    stream << (Q_UINT64) dbid;              // unique bit (64 bit)
    stream << (Q_UINT8) 0;                 // checked in iTZnes
    stream << (Q_UINT8) 0;                 // application rating
    stream << (Q_UINT16) 0;                 // BPM
    stream << (Q_UINT16) 0;                 // artworkcount
    stream << (Q_UINT16) 0xffff;            // unkown
    stream << (Q_UINT32) 0;                 // artwork size
    stream << (Q_UINT32) 0;                 // unkown
    stream << (float) -getSamplerate();      // samplerate as floating point "-"?!?
    stream << (Q_UINT32) 0;                 // date/time added
    stream << (Q_UINT32) file_format_code;  // unkown, but 0x0000000c for MP3 ?
    stream << (Q_UINT32) 0;                 // unkown
    stream << (Q_UINT32) 0;                 // unkown
    stream << (Q_UINT32) 0;                 // unkown
    stream << (Q_UINT32) 0;                 // unkown
    stream << (Q_UINT32) 0x02;              // unknown
    stream << (Q_UINT64) dbid; // same unique id as above
    for( int i= 0; i< 17; i++)
        stream << (Q_UINT32) 0;
    
    /** Write Track contents **/
    Q_UINT32 num_mhods = 0;
    for( PropertyMap::const_iterator element= properties.begin(); element!= properties.end(); ++element) {
        if( (*element).isEmpty())
            continue;

        const char *data= (const char *)(*element).ucs2();
        if( data == NULL)
            continue;

        int datalen= 2* (*element).length();

        stream << (Q_UINT32) 0x646F686D;    // mhod
        stream << (Q_UINT32) 0x18;    // headerlen
        stream << (Q_UINT32) 40+ datalen;
        stream << (Q_UINT32) element.key();
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 1;    // dummy - would refer to the trackID if used in playlist
        stream << (Q_UINT32) datalen;
        stream << (Q_UINT32) 0;
        stream << (Q_UINT32) 0;
        stream.writeRawBytes( data, datalen);
        num_mhods++;
    }
    buffer.at( 8);
    stream << (Q_UINT32)data.size();	// set content length
    stream << (Q_UINT32)num_mhods;	// set real mhod count
    buffer.close();
}

QDataStream & Track::writeToStream(QDataStream & outstream) {
    QByteArray buffer;
    writeData(buffer);
    outstream.writeRawBytes( buffer.data(), buffer.size());

    return outstream;
}

// TODO rename to readTagInStream
QDataStream & Track::readFromStream(QDataStream& instream) {
    Q_UINT32 dummy, blocklen;
    instream >> blocklen;

    // TODO make this more forgiving
    if(blocklen < 148) {
        // insufficient information
        QByteArray buffer( blocklen );
        instream.readRawBytes(buffer.data(), blocklen);
        return instream;
    }

    instream >> dummy;
    instream >> num_mhod;
    instream >> id;

    instream >> dummy;
    instream >> dummy;
    instream >> vbr;
    instream >> type;
    instream >> compilation;
    instream >> rating;
    instream >> lastmodified;
    lastmodified-= MAC_EPOCH_DELTA;
    instream >> file_size;

    instream >> tracklen;
    instream >> tracknum;
    instream >> numtracks;
    instream >> year;
    instream >> bitrate;

    instream >> samplerate;
    instream >> volumeadjust;
    instream >> dummy;
    instream >> dummy;
    instream >> dummy;

    instream >> playcount;
    instream >> dummy;
    instream >> last_played_at;
    instream >> cdnum;
    instream >> numcds;

    instream >> dummy;
    instream >> date_added;

    // 108 byte read so far

    if ( blocklen==156 ) // iTunes 4.7
    {
        for (int i= 0; i< 9; i++)
            instream >> dummy;
        instream >> file_format_code;
        QByteArray buffer(8);
        instream.readRawBytes(buffer.data(), 8);
    }
    else
    {
        if ( blocklen==244 ) // iTunes 4.9
        {
            instream >> dummy;
            instream >> dbid;
            for (int i= 0; i< 6; i++)
                instream >> dummy;
            instream >> file_format_code;
            QByteArray buffer(96);
            instream.readRawBytes(buffer.data(), 96);
        }
        else
        {
            // seek to the end of the block
            if (blocklen > 108) {
                QByteArray buffer(blocklen - 108);
                instream.readRawBytes(buffer.data(), blocklen - 108);
            }
        }
    }
    return instream;
}

const Q_UINT32 & Track::getID() const {
    return id;
}

/*!
    \fn itunesdb::Track::getAlbum()
 */
const QString & Track::getAlbum() const
{
    return getItemProperty( MHOD_ALBUM);
}


/*!
    \fn itunesdb::Track::getArtist()
 */
const QString & Track::getArtist() const
{
    return getItemProperty( MHOD_ARTIST);
}


/*!
    \fn itunesdb::Track::getComment()
 */
const QString & Track::getComment() const
{
    return getItemProperty( MHOD_COMMENT);
}


/*!
    \fn itunesdb::Track::getTitle()
 */
const QString & Track::getTitle() const
{
    return getItemProperty( MHOD_TITLE);
}


/*!
    \fn itunesdb::Track::getGenre()
 */
const QString & Track::getGenre() const
{
    return getItemProperty( MHOD_GENRE);
}


/*!
    \fn itunesdb::Track::getPath()
 */
const QString & Track::getPath() const
{
    return getItemProperty( itunesdb::MHOD_PATH);
}

/*!
    \fn itunesdb::Track::getComposer()
 */
const QString & Track::getComposer() const
{
    return getItemProperty( itunesdb::MHOD_COMPOSER);
}

Q_UINT32 Track::getNumMhod() const {
    return getNumComponents();
}

unsigned char Track::getRating() const {
    return rating;
}

Q_UINT32 Track::getLastModified() const {
    return lastmodified;
}

Q_UINT32 Track::getFileSize() const {
    return file_size;
}

Q_UINT32 Track::getTrackLength() const {
    return tracklen;
}

Q_UINT32 Track::getTrackNumber() const {
    return tracknum;
}

Q_UINT32 Track::getTrackCount() const {
    return numtracks;
}

Q_UINT32 Track::getYear() const {
    return year;
}

Q_UINT32 Track::getBitrate() const {
    return bitrate;
}

Q_UINT32 Track::getSamplerate() const {
    return samplerate;
}

Q_UINT32 Track::getVolumeAdjust() const {
    return volumeadjust;
}

Q_UINT32 Track::getPlayCount() const {
    return playcount;
}

Q_UINT32 Track::getLastPlayed() const {
    return last_played_at;
}

Q_UINT32 Track::getCdNumber() const {
    return cdnum;
}

Q_UINT32 Track::getCdCount() const {
    return numcds;
}


/**
 */
Q_UINT64 Track::getDBID() const
{
    return dbid;
}


/*!
    \fn TrackMetadata::setAlbum(QString& album)
 */
void Track::setAlbum(const QString& album)
{
    setItemProperty( album, itunesdb::MHOD_ALBUM);
}


/*!
    \fn TrackMetadata::setArtist(QString& artist)
 */
void Track::setArtist(const QString& artist)
{
    setItemProperty( artist, itunesdb::MHOD_ARTIST);
}


/*!
    \fn TrackMetadata::setGenre(QString& genre)
 */
void Track::setGenre(const QString& genre)
{
    setItemProperty( genre, itunesdb::MHOD_GENRE);
}


/*!
    \fn TrackMetadata::setTitle(QString& title)
 */
void Track::setTitle(const QString& title)
{
    setItemProperty( title, itunesdb::MHOD_TITLE);
}


void Track::setFDesc(const QString& fdesc) {
    setItemProperty( fdesc, itunesdb::MHOD_FDESC);
}

void Track::setComposer(const QString& composer) {
    setItemProperty( composer, itunesdb::MHOD_COMPOSER);
}

void Track::setTrackNumber(Q_UINT32 tracknumber) {
    tracknum = tracknumber;
}

/*!
    \fn itunesdb::Track::setNumTracksInAlbum(uint numtracks)
 */
void itunesdb::Track::setNumTracksInAlbum(Q_UINT32 newnumtracks) {
    numtracks = newnumtracks;
}


/**
 */
void Track::setDBID( Q_UINT64 id )
{
    dbid = id;
}


/*!
    \fn TrackMetadata::setComment(QString& comment)
 */
void Track::setComment(const QString& comment)
{
    setItemProperty( comment, itunesdb::MHOD_COMMENT);
}


/*!
    \fn TrackMetadata::setPath(QString& encodedpath)
 */
void Track::setPath(const QString& encodedpath)
{
    setItemProperty( encodedpath, itunesdb::MHOD_PATH);
}


void Track::doneAddingData() {
    // some checks
    if( getItemProperty(MHOD_ARTIST).isEmpty())
        setItemProperty("_no_artist_", MHOD_ARTIST);
    if( getItemProperty(MHOD_ALBUM).isEmpty())
        setItemProperty("_no_album_", MHOD_ALBUM);
    if( getItemProperty(MHOD_TITLE).isEmpty())
        setItemProperty("_no_title_", MHOD_TITLE);
}
}


