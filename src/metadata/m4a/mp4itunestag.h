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

#ifndef MP4ITUNESTAG_H
#define MP4ITUNESTAG_H

#include "taglib.h"
#include "tstring.h"
#include "tag.h"

namespace TagLib
{
  namespace MP4
  {
    class File;

    class Tag : public TagLib::Tag
    {
    public:
      /*!
       * Constructs an empty MP4 iTunes tag.
       */
      Tag( );
  
      /*!
       * Destroys this Tag instance.
       */
      virtual ~Tag();
  
      // Reimplementations.
  
      virtual String title() const;
      virtual String artist() const;
      virtual String album() const;
      virtual String comment() const;
      virtual String genre() const;
      virtual uint year() const;
      virtual uint track() const;
  
      virtual void setTitle(const String &s);
      virtual void setArtist(const String &s);
      virtual void setAlbum(const String &s);
      virtual void setComment(const String &s);
      virtual void setGenre(const String &s);
      virtual void setYear(const uint i);
      virtual void setTrack(const uint i);

      // MP4 specific fields

      String     grouping() const;
      String     composer() const;
      uint       disk() const;
      uint       bpm() const;
      ByteVector cover() const;
      int        compilation() const { return -1; }
      
      void setGrouping(const String &s);
      void setComposer(const String &s);
      void setDisk(const uint i);
      void setBpm(const uint i);
      void setCover( const ByteVector& cover );
      void setCompilation( bool /*isCompilation*/ ) {}
  
      virtual bool isEmpty() const;
  
    private:
      Tag(const Tag &);
      Tag &operator=(const Tag &);
  
      class TagPrivate;
      TagPrivate *d;
    };
  } // namespace MP4

} // namespace TagLib

#endif // MP4ITUNESTAG_H
