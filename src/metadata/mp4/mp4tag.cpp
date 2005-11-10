
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#include "mp4tag.h"

#include <tag.h>
#include <mp4.h>
#include <stdint.h>

using namespace TagLib;

MP4::Tag::Tag() : TagLib::Tag::Tag() {
    m_title = String::null;
    m_artist = String::null;
    m_album = String::null;
    m_comment = String::null;
    m_genre = String::null;
    m_year = 0;
    m_track = 0;
}

MP4::Tag::~Tag() {
}

bool MP4::Tag::isEmpty() const {
    return  m_title == String::null &&
        m_artist == String::null &&
        m_album == String::null && 
        m_comment == String::null &&
        m_genre == String::null &&
        m_year == 0 &&
        m_track == 0;
}

void MP4::Tag::duplicate(const Tag *source, Tag *target, bool overwrite) {
    // No nonstandard information stored yet
    Tag::duplicate(source, target, overwrite);
}

void MP4::Tag::readTags( MP4FileHandle mp4file )
{
    // Now parse tag.
    char *value;
    uint16_t numvalue, numvalue2;
    if (MP4GetMetadataName(mp4file, &value) && value != NULL) {
        m_title = String(value, String::UTF8);
        free(value);
    }
    if (MP4GetMetadataArtist(mp4file, &value) && value != NULL) {
        m_artist = String(value, String::UTF8);
        free(value);
    }

    if (MP4GetMetadataComment(mp4file, &value) && value != NULL) {
        m_comment = String(value, String::UTF8);
        free(value);
    }

    if (MP4GetMetadataYear(mp4file, &value) && value != NULL) {
        m_year = strtol(value, NULL,0);
        free(value);
    }
    if (MP4GetMetadataAlbum(mp4file, &value) && value != NULL) {
        m_album  =  String(value, String::UTF8);
        free(value);
    }
    if (MP4GetMetadataTrack(mp4file, &numvalue, &numvalue2)) {
        m_track = numvalue;
    }
    if (MP4GetMetadataGenre(mp4file, &value) && value != NULL) {
        m_genre = String(value, String::UTF8);
        free(value);
    }
}
