/***************************************************************************
    copyright            : (C) 2005 by Lukas Lalinsky
    email                : lalinsky@gmail.com
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#ifndef TAGLIB_WMATAG_H
#define TAGLIB_WMATAG_H

#include <tmap.h>
#include <tag.h>
#include <wmafile.h>
#include <wmaattribute.h>

namespace TagLib {

  namespace WMA {
  
    typedef Map<ByteVector, Attribute*> AttributeMap;
      
    class Tag : public TagLib::Tag {
    
      friend class File;
      
    public:
    
      Tag();

      virtual ~Tag();

      /*!
       * Returns the track name.
       */
      virtual String title() const;

      /*!
       * Returns the artist name.
       */
      virtual String artist() const;

      /*!
       * Returns the album name; if no album name is present in the tag
       * String::null will be returned.
       */
      virtual String album() const;

      /*!
       * Returns the track comment.
       */
      virtual String comment() const;

      /*!
       * Returns the genre name; if no genre is present in the tag String::null
       * will be returned.
       */
      virtual String genre() const;

      /*!
       * Returns the rating.
       */
      virtual String rating() const;

      /*!
       * Returns the genre name; if no genre is present in the tag String::null
       * will be returned.
       */
      virtual String copyright() const;

      /*!
       * Returns the year; if there is no year set, this will return 0.
       */
      virtual unsigned year() const;

      /*!
       * Returns the track number; if there is no track number set, this will
       * return 0.
       */
      virtual unsigned track() const;

      /*!
       * Sets the title to \a s.
       */
      virtual void setTitle(const String &s);

      /*!
       * Sets the artist to \a s.
       */
      virtual void setArtist(const String &s);

      /*!
       * Sets the album to \a s.  If \a s is String::null then this value will be
       * cleared.
       */
      virtual void setAlbum(const String &s);

      /*!
       * Sets the comment to \a s.
       */
      virtual void setComment(const String &s);

      /*!
       * Sets the rating to \a s. 
       */
      virtual void setRating(const String &s);

      /*!
       * Sets the copyright to \a s. 
       */
      virtual void setCopyright(const String &s);

      /*!
       * Sets the genre to \a s. 
       */
      virtual void setGenre(const String &s);

      /*!
       * Sets the year to \a i.  If \a s is 0 then this value will be cleared.
       */
      virtual void setYear(unsigned i);

      /*!
       * Sets the track to \a i.  If \a s is 0 then this value will be cleared.
       */
      virtual void setTrack(unsigned i);

      /*!
       * Returns true if the tag does not contain any data.  This should be
       * reimplemented in subclasses that provide more than the basic tagging
       * abilities in this class.
       */
      virtual bool isEmpty() const;

      /*!
       * Copies the generic data from one tag to another.
       *
       * If \a overwrite is true then the values will be unconditionally copied.
       * If false only empty values will be overwritten.
       */
      static void duplicate(const Tag *source, Tag *target, bool overwrite = true);

      const AttributeMap &attributeMap() const;
      
      void setAttribute(const ByteVector &name, const String &value);
      void setAttribute(const ByteVector &name, Attribute *attribute);

    private:
      
      class TagPrivate;
      TagPrivate *d;
      
    };
  }
}
#endif
