/***************************************************************************
    copyright            : (C) 2002, 2003, 2006 by Jochen Issing
    email                : jochen.issing@isign-softart.de
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include <tbytevector.h>
#include <tstring.h>
#include "tlist.h"

#include "mp4itunestag.h"
#include "mp4file.h"
#include "boxfactory.h"
#include "mp4tagsproxy.h"
#include "mp4propsproxy.h"
#include "mp4audioproperties.h"
#include "itunesdatabox.h"

using namespace TagLib;

class MP4::File::FilePrivate
{
public:
  //! list for all boxes of the mp4 file
  TagLib::List<MP4::Mp4IsoBox*> boxes;
  //! the box factory - will create all boxes by tag and size
  MP4::BoxFactory             boxfactory;
  //! proxy for the tags is filled after parsing
  MP4::Mp4TagsProxy           tagsProxy;
  //! proxy for audio properties
  MP4::Mp4PropsProxy          propsProxy;
  //! the tag returned by tag() function
  MP4::Tag                    mp4tag;
  //! container for the audio properties returned by properties() function
  MP4::AudioProperties        mp4audioproperties;
  //! is set to valid after successfully parsing
  bool                        isValid;
};

//! function to fill the tags with converted proxy data, which has been parsed out of the file previously
static void fillTagFromProxy( MP4::Mp4TagsProxy& proxy, MP4::Tag& mp4tag );

MP4::File::File(const char *file, bool , AudioProperties::ReadStyle  )
	:TagLib::File( file )
{
  // create member container
  d = new MP4::File::FilePrivate();


  d->isValid = false;
  TagLib::uint size;
  MP4::Fourcc  fourcc;

  while( readSizeAndType( size, fourcc ) == true )
  {
    // create the appropriate subclass and parse it
    MP4::Mp4IsoBox* curbox = d->boxfactory.createInstance( this, fourcc, size, tell() );
    curbox->parsebox();
    d->boxes.append( curbox );
  }

  for( TagLib::List<MP4::Mp4IsoBox*>::Iterator iter  = d->boxes.begin();
                                               iter != d->boxes.end();
					       iter++ )
  {
    if( (*iter)->fourcc() == MP4::Fourcc("moov") )
    {
      d->isValid = true;
      break;
    }
  }
   
  //if( d->isValid )
    //debug( "file is valid" );
  //else
    //debug( "file is NOT valid" );

  // fill tags from proxy data
  fillTagFromProxy( d->tagsProxy, d->mp4tag );
}

MP4::File::~File()
{
  TagLib::List<Mp4IsoBox*>::Iterator delIter;
  for( delIter  = d->boxes.begin();
       delIter != d->boxes.end();
       delIter++ )
  {
    delete *delIter;
  }
  delete d;
}

Tag *MP4::File::tag() const
{
  return &d->mp4tag;
}

AudioProperties * MP4::File::audioProperties() const
{
  d->mp4audioproperties.setProxy( &d->propsProxy );
  return &d->mp4audioproperties;
}

bool MP4::File::save()
{
  return false;
}

void MP4::File::remove()
{
}

TagLib::uint MP4::File::readSystemsLen()
{
  TagLib::uint length = 0;
  TagLib::uint nbytes = 0;
  ByteVector   input;
  TagLib::uchar tmp_input;

  do
  {
    input = readBlock(1);
    tmp_input = static_cast<TagLib::uchar>(input[0]);
    nbytes++;
    length = (length<<7) | (tmp_input&0x7F);
  } while( (tmp_input&0x80) && (nbytes<4) );

  return length;
}

bool MP4::File::readSizeAndType( TagLib::uint& size, MP4::Fourcc& fourcc )
{
  // read the two blocks from file
  ByteVector readsize = readBlock(4);
  ByteVector readtype = readBlock(4);

  if( (readsize.size() != 4) || (readtype.size() != 4) )
    return false;

  // set size
  size = static_cast<unsigned char>(readsize[0]) << 24 | 
         static_cast<unsigned char>(readsize[1]) << 16 |
         static_cast<unsigned char>(readsize[2]) <<  8 |
         static_cast<unsigned char>(readsize[3]);

  // type and size seem to be part of the stored size
  if( size < 8 )
    return false;

  // set fourcc
  fourcc = readtype.data();

  return true;
}

bool MP4::File::readInt( TagLib::uint& toRead )
{
  ByteVector readbuffer = readBlock(4);
  if( readbuffer.size() != 4 )
    return false;

  toRead = static_cast<unsigned char>(readbuffer[0]) << 24 | 
           static_cast<unsigned char>(readbuffer[1]) << 16 |
           static_cast<unsigned char>(readbuffer[2]) <<  8 |
           static_cast<unsigned char>(readbuffer[3]);
  return true;
}

bool MP4::File::readShort( TagLib::uint& toRead )
{
  ByteVector readbuffer = readBlock(2);
  if( readbuffer.size() != 2 )
    return false;

  toRead = static_cast<unsigned char>(readbuffer[0]) <<  8 |
           static_cast<unsigned char>(readbuffer[1]);
  return true;
}

bool MP4::File::readLongLong( TagLib::ulonglong& toRead )
{
  ByteVector readbuffer = readBlock(8);
  if( readbuffer.size() != 8 )
    return false;

  toRead = static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[0])) << 56 | 
           static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[1])) << 48 |
           static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[2])) << 40 |
           static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[3])) << 32 |
           static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[4])) << 24 | 
           static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[5])) << 16 |
           static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[6])) <<  8 |
           static_cast<ulonglong>(static_cast<unsigned char>(readbuffer[7]));
  return true;
}

bool MP4::File::readFourcc( TagLib::MP4::Fourcc& fourcc )
{
  ByteVector readtype = readBlock(4);

  if( readtype.size() != 4)
    return false;

  // set fourcc
  fourcc = readtype.data();

  return true;
}

MP4::Mp4TagsProxy* MP4::File::tagProxy() const
{
  return &d->tagsProxy;
}

MP4::Mp4PropsProxy* MP4::File::propProxy() const
{
  return &d->propsProxy;
}

void fillTagFromProxy( MP4::Mp4TagsProxy& proxy, MP4::Tag& mp4tag )
{
  // tmp buffer for each tag
  MP4::ITunesDataBox* databox;
  
  databox = proxy.titleData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setTitle( datastring );
  }

  databox = proxy.artistData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setArtist( datastring );
  }

  databox = proxy.albumData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setAlbum( datastring );
  }

  databox = proxy.genreData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setGenre( datastring );
  }

  databox = proxy.yearData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setYear( datastring.toInt() );
  }

  databox = proxy.trknData();
  if( databox != 0 )
  {
    // convert data to uint
    TagLib::ByteVector datavec = databox->data();
    if( datavec.size() >= 4 )
    {
      TagLib::uint trackno = static_cast<TagLib::uint>( static_cast<unsigned char>(datavec[0]) << 24 |
	                                                static_cast<unsigned char>(datavec[1]) << 16 |
	                                                static_cast<unsigned char>(datavec[2]) <<  8 |
	                                                static_cast<unsigned char>(datavec[3]) );
      mp4tag.setTrack( trackno );
    }
    else
      mp4tag.setTrack( 0 );
  }

  databox = proxy.commentData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setComment( datastring );
  }

  databox = proxy.groupingData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setGrouping( datastring );
  }

  databox = proxy.composerData();
  if( databox != 0 )
  {
    // convert data to string
    TagLib::String datastring( databox->data(), String::UTF8 );
    // check if string was set
    if( !datastring.isEmpty() )
      mp4tag.setComposer( datastring );
  }

  databox = proxy.diskData();
  if( databox != 0 )
  {
    // convert data to uint
    TagLib::ByteVector datavec = databox->data();
    if( datavec.size() >= 4 )
    {
      TagLib::uint discno = static_cast<TagLib::uint>( static_cast<unsigned char>(datavec[0]) << 24 |
	                                               static_cast<unsigned char>(datavec[1]) << 16 |
	                                               static_cast<unsigned char>(datavec[2]) <<  8 |
	                                               static_cast<unsigned char>(datavec[3]) );
      mp4tag.setDisk( discno );
    }
    else
      mp4tag.setDisk( 0 );
  }

  databox = proxy.bpmData();
  if( databox != 0 )
  {
    // convert data to uint
    TagLib::ByteVector datavec = databox->data();

    if( datavec.size() >= 2 )
    {
      TagLib::uint bpm = static_cast<TagLib::uint>( static_cast<unsigned char>(datavec[0]) <<  8 |
	                                            static_cast<unsigned char>(datavec[1]) );
      mp4tag.setBpm( bpm );
    }
    else
      mp4tag.setBpm( 0 );
  }

  databox = proxy.coverData();
  if( databox != 0 )
  {
    // get byte vector
    mp4tag.setCover( databox->data() );
  }
}
