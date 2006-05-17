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
        unsigned NjbTrack::getId() const { return id; }
        unsigned NjbTrack::getSize() const { return size; }
        unsigned NjbTrack::getDuration() const { return duration; }
        unsigned NjbTrack::getTrackNum() const { return tracknum; }
        QString NjbTrack::getYear() const { return year; }
        QString NjbTrack::getGenre() const { return genre; }
        QString NjbTrack::getArtist() const { return artist; }
        QString NjbTrack::getAlbum() const { return album; }
        QString NjbTrack::getTitle() const { return title; }
        QString NjbTrack::getCodec() const { return codec; }
        QString NjbTrack::getFilename() const { return filename; }

        void NjbTrack::setId( unsigned newId) { id = newId; }
        void NjbTrack::setSize( unsigned newSize) { size = newSize; }
        void NjbTrack::setDuration( unsigned newDuration) { duration = newDuration; }
        void NjbTrack::setTrackNum( unsigned newTrackNum) { tracknum = newTrackNum; }
        void NjbTrack::setYear( QString newYear) { year = newYear; }
        void NjbTrack::setGenre( QString newGenre) { genre = newGenre; }
        void NjbTrack::setArtist( QString newArtist) { artist = newArtist; }
        void NjbTrack::setAlbum( QString newAlbum) { album = newAlbum; }
        void NjbTrack::setTitle( QString newTitle) { title = newTitle; }
        void NjbTrack::setCodec( QString newCodec) { codec = newCodec; }
        void NjbTrack::setFilename( QString newFilename) { filename = newFilename; }

        MetaBundle* NjbTrack::getMetaBundle();

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
