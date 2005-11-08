// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

#ifndef TAGLIB_AUDIBLETAG_H
#define TAGLIB_AUDIBLETAG_H

#include <config.h>

#include <taglib/tag.h>
#include "taglib_audiblefile.h"

namespace TagLib {

    namespace Audible {
        /*!
         * This implements the generic TagLib::Tag API
         */
        class Tag : public TagLib::Tag
        {
            public:
                Tag();

                /*!
                 * read tags from the aa file.
                 */
                void readTags( FILE *file );

                /*!
                 * Destroys this AudibleTag instance.
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
                 * Returns the year; if there is no year set, this will return 0.
                 */
                virtual uint year() const { return m_year; }

                /*!
                 * Returns the track number; if there is no track number set, this will
                 * return 0.
                 */
                virtual uint track() const { return m_track; }

                /*!
                 * Returns the user id for this file.
                 */
                virtual uint userID() const { return m_userID; }

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
                 * Sets the track to \a i.  If \a s is 0 then this value will be cleared.
                 */
                virtual void setTrack(uint i) { m_track = i; }

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

                virtual void setUserID(uint id) { m_userID = id; }

                int getTagsEndOffset();



            protected:
                String m_title;
                String m_artist;
                String m_album;
                String m_comment;
                String m_genre;
                uint m_year;
                uint m_track;
                uint m_userID;
                bool readTag( FILE *fp, char **name, char **value);
                int m_tagsEndOffset;
        };
    }
}

#endif
