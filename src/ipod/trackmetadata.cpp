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

#include "trackmetadata.h"
#include "tracklist.h"

#include "../metabundle.h"

#include <qfileinfo.h>
#include <qstringlist.h>


TrackMetadata::TrackMetadata()
  : itunesdb::Track()
{
  samplerate = 0xAC440000;    // hardwired
  setFDesc( QString( "MPEG audio file"));    // hardwired for now
}


TrackMetadata::TrackMetadata(Q_UINT32 trackid)
  : itunesdb::Track()
{
  id = trackid;
  samplerate = 0xAC440000;    // hardwired
  setFDesc( QString( "MPEG audio file"));    // hardwired for now
}


TrackMetadata::TrackMetadata(const Track& track)
  : itunesdb::Track(track)
{
  samplerate = 0xAC440000;    // hardwired
  setFDesc( QString( "MPEG audio file"));    // hardwired for now
}

TrackMetadata::~TrackMetadata()
{
}

/*!
    \fn TrackMetadata::getFileExtension()
 */
const QString& TrackMetadata::getFileExtension() const
{
  return file_extension;
}

/*!
    \fn TrackMetadata::setFileExtension( QString& extension)
 */
void TrackMetadata::setFileExtension(const QString& extension)
{
  file_extension= extension;
}


/*!
    \fn TrackMetadata::readFromFile(const QString& filename)
 */
bool TrackMetadata::readFromBundle( const MetaBundle& bundle )
{
    QFileInfo fileinfo( bundle.url().path() );
    if( !fileinfo.exists() )
        return false;    // whatever happened here

    setFileExtension( fileinfo.extension( false ) );
    file_size = fileinfo.size();
    lastmodified = fileinfo.lastModified().toTime_t();

    volumeadjust = 0;
    playcount = 0;
    setAlbum( bundle.album().isEmpty() ? i18n( "Unknown" ) : bundle.album() );
    setArtist( bundle.artist() );
    setTitle( bundle.title() );
    setComment( "" );
    setGenre( "" );

    tracklen = bundle.length() * 1000;
    tracknum = bundle.track().toInt();
    bitrate = bundle.bitrate();
    vbr = 0;
    type = 1;
    compilation = 0;
    rating = 0;

    // some sanity checks
    if (getArtist().isEmpty() || getAlbum().isEmpty() || getTitle().isEmpty())
        return false;    // for now

    doneAddingData();

    return true;
}


/*!
    \fn TrackMetadata::toLogEntry()
 */
QStringList& TrackMetadata::toLogEntry(QStringList& valuebuffer) const
{
  valuebuffer << QString().setNum( getID() );
  valuebuffer << getPath();
  valuebuffer << getArtist();
  valuebuffer << getAlbum();
  valuebuffer << getTitle();
  valuebuffer << getComment();
  valuebuffer << getGenre();
  valuebuffer << getComposer();
  valuebuffer << QString().setNum( getYear());
  valuebuffer << QString().setNum( getFileSize());
  valuebuffer << QString().setNum( getLastModified());
  valuebuffer << QString().setNum( getBitrate());
  valuebuffer << QString().setNum( getSamplerate());
  valuebuffer << QString().setNum( getTrackLength());
  valuebuffer << QString().setNum( getTrackNumber());
  valuebuffer << QString().setNum( getCdNumber());
  valuebuffer << QString().setNum( getCdCount());
  valuebuffer << QString().setNum( getVolumeAdjust());
  valuebuffer << getFileExtension();
  valuebuffer << QString().setNum( vbr);
  valuebuffer << QString().setNum( type);
  valuebuffer << QString().setNum( compilation);
  valuebuffer << QString().setNum( rating);
  valuebuffer << QString().setNum( playcount);

  return valuebuffer;
}


/*!
    \fn TrackMetadata::readFromLogEntry()
 */
bool TrackMetadata::readFromLogEntry(const QStringList& values)
{
    // some sanity checks first
  if (values.size() < 19)
    return false;    // wtf?

  for( uint i= 0; i < values.size(); i++) {
    QString data( values[ i ]);
    switch( i) {
      case 0:
        id = data.toUInt();
        if (id == 0)
          return false;
        break;
        case 1: setPath( data); break;
        case 2: setArtist( data); break;
        case 3: setAlbum( data); break;
        case 4: setTitle( data); break;
        case 5: setComment( data); break;
        case 6: setGenre( data); break;
        case 7: setComposer( data); break;
        case 8: year = data.toUInt(); break;
        case 9: file_size = data.toUInt(); break;
        case 10: lastmodified = data.toUInt(); break;
        case 11: bitrate = data.toUInt(); break;
        case 12: samplerate = data.toUInt(); break;
        case 13: tracklen = data.toUInt(); break;
        case 14: tracknum = data.toUInt(); break;
        case 15: cdnum = data.toUInt(); break;
        case 16: numcds = data.toUInt(); break;
        case 17: volumeadjust = data.toUInt(); break;
        case 18: setFileExtension( data); break;
        case 19: vbr = data.toUShort() & 255; break;
        case 20: type = data.toUShort() & 255; break;
        case 21: compilation = data.toUShort() & 255; break;
        case 22: rating = data.toUShort() & 255; break;
        case 23: playcount = data.toUInt(); break;
    }
  }

  doneAddingData();

  return true;
}

