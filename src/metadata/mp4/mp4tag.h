/***************************************************************************
copyright            : (C) 2005 by Andy Leadbetter
email                : andrew.leadbetter@gmail.com
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
#ifndef TAGLIB_MP4TAG_H
#define TAGLIB_MP4TAG_H

#include <tag.h>
#include "mp4file.h"
#include <mp4.h>

namespace TagLib {

    namespace MP4 {
        /*!
         * This implements the generic TagLib::Tag API
         */
        class Tag : public TagLib::Tag
        {
            public:
                static const int Undefined = -1;
                
                Tag();

                /*!
                 * read tags from the mp4 file.
                 */
                void readTags( MP4FileHandle mp4file);

                /*!
                 * Destroys this MP4Tag instance.
                 */
                virtual ~Tag();

                /*!
                 * Returns the track name; if no track name is present in the tag
                 * String::null will be returned.
                 */
                virtual String title() const { return m_title; }

                /*!
                 * Returns the artist name; if no artist name is present in the tag
                 * String::null will be returned.
                 */
                virtual String artist() const { return m_artist; }

                /*!
                 * Returns the album name; if no album name is present in the tag
                 * String::null will be returned.
                 */
                virtual String album() const { return m_album; }

                /*!
                 * Returns the track comment; if no comment is present in the tag
                 * String::null will be returned.
                 */
                virtual String comment() const { return m_comment; }

                /*!
                 * Returns the genre name; if no genre is present in the tag String::null
                 * will be returned.
                 */
                virtual String genre() const { return m_genre; }

                /*!
                 * Returns the composer name; if no composer is present in the tag String::null
                 * will be returned.
                 */
                virtual String composer() const { return m_composer; }

                /*!
                 * Returns the year; if there is no year set, this will return 0.
                 */
                virtual uint year() const { return m_year; }

                /*!
                 * Returns the track number; if there is no track number set, this will
                 * return 0.
                 */
                virtual uint track() const { return m_track; }

                /*!
                 * Returns the disc number; if there is no disc number set, this will
                 * return 0.
                 */
                virtual uint disk() const { return m_disk; }

                /*!
                 * Returns the BPM (tempo);  if there is no BPM, this will return 0.
                 */
                virtual uint bpm() const { return m_bpm; }

                /*!
                * Returns the embedded cover image; if there is no cover set, this will
                * return an empty ByteVector.
                */
                virtual const ByteVector &cover() const { return m_image; }

                /*!
                 * Returns whether this is part of a compilation; if this flag is not set,
                 * this will return the Undefined constant.
                 */
                virtual int compilation() const { return m_compilation; }

                /*!
                 * Sets the title to \a s.  If \a s is String::null then this value will be
                 * cleared.
                 */
                virtual void setTitle(const String &s) { m_title = s; }

                /*!
                 * Sets the artist to \a s.  If \a s is String::null then this value will be
                 * cleared.
                 */
                virtual void setArtist(const String &s) { m_artist = s; }

                /*!
                 * Sets the album to \a s.  If \a s is String::null then this value will be
                 * cleared.
                 */
                virtual void setAlbum(const String &s) { m_album = s; } 

                /*!
                 * Sets the album to \a s.  If \a s is String::null then this value will be
                 * cleared.
                 */
                virtual void setComment(const String &s) { m_comment = s; }

                /*!
                 * Sets the genre to \a s.  If \a s is String::null then this value will be
                 * cleared.  For tag formats that use a fixed set of genres, the appropriate
                 * value will be selected based on a string comparison.  A list of available
                 * genres for those formats should be available in that type's
                 * implementation.
                 */
                virtual void setGenre(const String &s) { m_genre = s; }

                /*!
                 * Sets the year to \a i.  If \a s is 0 then this value will be cleared.
                 */
                virtual void setYear(uint i) { m_year = i; }

                /*!
                 * Sets the track to \a i.  If \a i is 0 then this value will be cleared.
                 */
                virtual void setTrack(uint i) { m_track = i; }

                /*!
                 * Sets the disc to \a i.  If \a i is 0 then this value will be cleared.
                 */
                virtual void setDisk(uint i) { m_disk = i; }

                /*!
                 * Sets the BPM (tempo) to \a i.  It \a i is 0 then this value will be cleared.
                 */
                virtual void setBpm(uint i) { m_bpm = i; }

                /*!
                 * Sets whether this is part of a compilation.
                 */
                virtual void setCompilation(bool compilation) { m_compilation = compilation ? 1 : 0; }

                /*!
                 * Sets the composer to \a s.  If \a s is String::null then this value will
                 * be cleared.
                 */
                virtual void setComposer(const String &s) { m_composer = s; }

                /*!
                 * Sets the embedded cover image to \a i. If \a i is empty then this value
                 * will be cleared.
                 */
                virtual void setCover(const ByteVector &i) { m_image = i; }

                /*!
                 * Returns true if the tag does not contain any data.  This should be
                 * reimplemented in subclasses that provide more than the basic tagging
                 * abilities in this class.
                 */
                virtual bool isEmpty() const;

                /*!
                 * Copies the generic data from one tag to another.
                 *
                 * \note This will not affect any of the lower level details of the tag.  For
                 * instance if any of the tag type specific data (maybe a URL for a band) is
                 * set, this will not modify or copy that.  This just copies using the API
                 * in this class.
                 *
                 * If \a overwrite is true then the values will be unconditionally copied.
                 * If false only empty values will be overwritten.
                 */
                static void duplicate(const Tag *source, Tag *target, bool overwrite = true);

            protected:
                String m_title;
                String m_artist;
                String m_album;
                String m_comment;
                String m_genre;
                String m_composer;
                uint m_year;
                uint m_track;
                uint m_disk;
                uint m_bpm;
                int m_compilation;
                ByteVector m_image;
        };
    }
}
#endif
