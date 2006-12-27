
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

#include "mp4tag.h"

#include <tag.h>
#include <stdint.h>

using namespace TagLib;

MP4::Tag::Tag() : TagLib::Tag::Tag() {
    m_title = String::null;
    m_artist = String::null;
    m_album = String::null;
    m_comment = String::null;
    m_genre = String::null;
    m_composer = String::null;
    m_year = 0;
    m_track = 0;
    m_disk = 0;
    m_bpm = 0;
    m_compilation = Undefined;
}

MP4::Tag::~Tag() {
}

bool MP4::Tag::isEmpty() const {
    return  m_title == String::null &&
        m_artist == String::null &&
        m_album == String::null && 
        m_comment == String::null &&
        m_genre == String::null &&
        m_composer == String::null &&
        m_year == 0 &&
        m_track == 0 &&
        m_disk == 0 &&
        m_bpm == 0 &&
        m_compilation == Undefined &&
        m_image.size() == 0;
}

void MP4::Tag::duplicate(const Tag *source, Tag *target, bool overwrite) {
    // Duplicate standard information
    Tag::duplicate(source, target, overwrite);

    if (overwrite || target->compilation() == Undefined && source->compilation() != Undefined)
        target->setCompilation(source->compilation());

    if (overwrite || target->cover().size() == 0)
        target->setCover(source->cover());
}

void MP4::Tag::readTags( MP4FileHandle mp4file )
{
    // Now parse tag.
    char *value;
    uint8_t boolvalue;
    uint16_t numvalue, numvalue2;
    uint8_t *image;
    uint32_t imageSize;
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
    if (MP4GetMetadataDisk(mp4file, &numvalue, &numvalue2)) {
        m_disk = numvalue;
    }
    if (MP4GetMetadataTempo(mp4file, &numvalue)) {
        m_bpm = numvalue;
    }
    if (MP4GetMetadataCompilation(mp4file, &boolvalue)) {
        m_compilation = boolvalue;
    }
    if (MP4GetMetadataGenre(mp4file, &value) && value != NULL) {
        m_genre = String(value, String::UTF8);
        free(value);
    }
    if (MP4GetMetadataWriter(mp4file, &value) && value != NULL) {
        m_composer = String(value, String::UTF8);
        free(value);
    }
    if (MP4GetMetadataCoverArt(mp4file, &image, &imageSize) && image && imageSize) {
        m_image.setData(reinterpret_cast<const char *>( image ), imageSize);
        free(image);
    }
}
