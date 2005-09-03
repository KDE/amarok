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
#ifndef ITUNESDBTRACK_H
#define ITUNESDBTRACK_H

#include <qcstring.h>

#include "listitem.h"

namespace itunesdb {

/**
represents a track

@author Michael Schulze
*/
class Track : public ListItem
{
public:
    Track();
    virtual ~Track();

    virtual const Q_UINT32 & getID() const;
    virtual const QString & getAlbum() const;
    virtual const QString & getArtist() const;
    virtual const QString & getComment() const;
    virtual const QString & getTitle() const;
    virtual const QString & getGenre() const;
    virtual const QString & getPath() const;
    virtual const QString & getComposer() const;
    virtual Q_UINT64 getDBID() const;
    virtual Q_UINT32 getNumMhod() const;
    virtual unsigned char getRating() const;
    virtual Q_UINT32 getLastModified() const;
    virtual Q_UINT32 getFileSize() const;
    virtual Q_UINT32 getTrackLength() const;
    virtual Q_UINT32 getTrackNumber() const;
    virtual Q_UINT32 getTrackCount() const;
    virtual Q_UINT32 getYear() const;
    virtual Q_UINT32 getBitrate() const;
    virtual Q_UINT32 getSamplerate() const;
    virtual Q_UINT32 getVolumeAdjust() const;
    virtual Q_UINT32 getPlayCount() const;
    virtual Q_UINT32 getLastPlayed() const;
    virtual Q_UINT32 getCdNumber() const;
    virtual Q_UINT32 getCdCount() const;

    virtual void setDBID( Q_UINT64 id );

    virtual void setAlbum(const QString& album);
    virtual void setArtist(const QString& artist);
    virtual void setPath(const QString& encodedpath);
    virtual void setComment(const QString& comment);
    virtual void setGenre(const QString& genre);
    virtual void setTitle(const QString& title);
    virtual void setFDesc(const QString& fdesc);
    virtual void setComposer(const QString& composer);
    virtual void setTrackNumber(Q_UINT32 tracknumber);
    virtual void setNumTracksInAlbum(Q_UINT32 newnumtracks);

    void doneAddingData();

    void writeData( QByteArray& data) const;

    QDataStream & writeToStream(QDataStream & outstream);
    QDataStream & readFromStream(QDataStream & instream);

protected:
    Q_UINT32 id;

    // attributes
    Q_UINT32 num_mhod;    // number of data items
    Q_UINT32 lastmodified;    // last modified date
    Q_UINT32 file_size;
    Q_UINT32 tracklen;
    Q_UINT32 tracknum;
    Q_UINT32 numtracks;
    Q_UINT32 year;
    Q_UINT32 bitrate;
    Q_UINT32 samplerate;
    Q_UINT32 volumeadjust;
    Q_UINT32 playcount;
    Q_UINT32 last_played_at;
    Q_UINT32 cdnum;
    Q_UINT32 numcds;
    Q_UINT32 file_format_code;
    Q_UINT32 date_added;
    unsigned char rating;       // rating 0 lowest 100 highest
    unsigned char vbr;          // vbr=1, cbr=0
    unsigned char type;         // mp3=1, aac+audible=0
    unsigned char compilation;  // iscompiltaion=1, 0 otherwise
    Q_UINT64 dbid;
};

}

#endif
