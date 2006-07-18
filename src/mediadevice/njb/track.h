/***************************************************************************
                           track.h  -  description
                             -------------------
    begin                : 2001-07-24
    copyright            : (C) 2001 by Shaun Jackman (sjackman@debian.org)
    modify by            : Andres Oton 
    email                : andres.oton@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __track_h__
#define __track_h__


// qt
#include <qstring.h>

// libnjb
#include <libnjb.h>

#include "metabundle.h"

class NjbTrack {
    public:
        NjbTrack( void) {}
        NjbTrack( njb_songid_t* song);
        NjbTrack( char** result);

        bool operator==( const NjbTrack& second ) const { return id == second.id; }

    public:
        unsigned getId() const { return id; }
        unsigned getSize() const { return size; }
        unsigned getDuration() const { return duration; }
        unsigned getTrackNum() const { return tracknum; }
        QString getYear() const { return year; }
        QString getGenre() const { return genre; }
        QString getArtist() const { return artist; }
        QString getAlbum() const { return album; }
        QString getTitle() const { return title; }
        QString getCodec() const { return codec; }
        QString getFilename() const { return filename; }

        void setId( unsigned newId) { id = newId; }
        void setSize( unsigned newSize) { size = newSize; }
        void setDuration( unsigned newDuration) { duration = newDuration; }
        void setTrackNum( unsigned newTrackNum) { tracknum = newTrackNum; }
        void setYear( QString newYear) { year = newYear; }
        void setGenre( QString newGenre) { genre = newGenre; }
        void setArtist( QString newArtist) { artist = newArtist; }
        void setAlbum( QString newAlbum) { album = newAlbum; }
        void setTitle( QString newTitle) { title = newTitle; }
        void setCodec( QString newCodec) { codec = newCodec; }
        void setFilename( QString newFilename) { filename = newFilename; }

        MetaBundle* getMetaBundle();

    private:
        unsigned id;
        unsigned size;
        unsigned duration;
        unsigned tracknum;
        QString year;
        QString genre;
        QString artist;
        QString album;
        QString title;
        QString codec;
        QString filename;

};

class trackValueList: public QValueList<NjbTrack>
{
    public:
        trackValueList::iterator findTrackByName( const QString& );
        trackValueList::const_iterator findTrackByName( const QString& ) const;
        trackValueList::iterator findTrackById( unsigned );
        trackValueList::const_iterator findTrackById( unsigned ) const;

        int readFromDevice( void);
};

#endif
