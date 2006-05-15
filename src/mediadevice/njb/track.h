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

class Track {
public:
	Track( void) {}
	Track( njb_songid_t* song);
	Track( char** result);

	bool operator==( const Track& second ) const { return id == second.id; }

public:
	unsigned Track::getId() const { return id; }
	unsigned Track::getSize() const { return size; }
	unsigned Track::getDuration() const { return duration; }
	unsigned Track::getTrackNum() const { return tracknum; }
	QString Track::getYear() const { return year; }
	QString Track::getGenre() const { return genre; }
	QString Track::getArtist() const { return artist; }
	QString Track::getAlbum() const { return album; }
	QString Track::getTitle() const { return title; }
	QString Track::getCodec() const { return codec; }
	QString Track::getFilename() const { return filename; }

	void Track::setId( unsigned newId) { id = newId; }
	void Track::setSize( unsigned newSize) { size = newSize; }
	void Track::setDuration( unsigned newDuration) { duration = newDuration; }
	void Track::setTrackNum( unsigned newTrackNum) { tracknum = newTrackNum; }
	void Track::setYear( QString newYear) { year = newYear; }
	void Track::setGenre( QString newGenre) { genre = newGenre; }
	void Track::setArtist( QString newArtist) { artist = newArtist; }
	void Track::setAlbum( QString newAlbum) { album = newAlbum; }
	void Track::setTitle( QString newTitle) { title = newTitle; }
	void Track::setCodec( QString newCodec) { codec = newCodec; }
	void Track::setFilename( QString newFilename) { filename = newFilename; }

	MetaBundle* Track::getMetaBundle();
	
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

class trackValueList: public QValueList<Track>
{
public:
	trackValueList::iterator findTrackByName( const QString& );
	trackValueList::const_iterator findTrackByName( const QString& ) const;
	trackValueList::iterator findTrackById( unsigned );
	trackValueList::const_iterator findTrackById( unsigned ) const;
	
	int readFromDevice( void);
};

#endif
