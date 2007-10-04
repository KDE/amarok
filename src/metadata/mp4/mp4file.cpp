/***************************************************************************
copyright            : (C) 2005 by Andy Leadbetter
email                : andrew.leadbetter@gmail.com

copyright            : (C) 2005 by Martin Aumueller
email                : aumuell@reserv.at
                       (write support)
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

#include "mp4file.h"

#include "mp4tag.h"
#include <tfile.h>
#include <audioproperties.h>

#include <stdint.h>

#define MP4V2_HAS_WRITE_BUG 1

namespace TagLib {
////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MP4::File::File(const char *file,
        bool readProperties,
        Properties::ReadStyle propertiesStyle,
        MP4FileHandle handle) : TagLib::File(file),
        mp4tag(NULL), properties(NULL)
{

    //   debug ("MP4::File: create new file object.");
    //debug ( file );
    /**
     * Create the MP4 file.
     */

    if(handle == MP4_INVALID_FILE_HANDLE)
    {
        mp4file = MP4Read(file);
    }
    else
    {
        mp4file = handle;
    }

    if( isOpen() )
    {
        read(readProperties, propertiesStyle );
    }
}

MP4::File::~File()
{
    MP4Close(mp4file);
    delete mp4tag;
    delete properties;
}

TagLib::Tag *MP4::File::tag() const
{
    return mp4tag;
}

TagLib::MP4::Tag *MP4::File::getMP4Tag() const
{
    return mp4tag;
}

MP4::Properties *MP4::File::audioProperties() const
{
    return properties;
}

bool MP4::File::save()
{
    MP4Close(mp4file);

    MP4FileHandle handle = MP4Modify(name());
    if(handle == MP4_INVALID_FILE_HANDLE)
    {
        mp4file = MP4Read(name());
        return false;
    }

#ifdef MP4V2_HAS_WRITE_BUG
    /* according to gtkpod we have to delete all meta data before modifying it,
       save the stuff we would not touch */

    // need to fetch/rewrite this only if we aren't going to anyway
    uint8_t compilation = 0;
    bool has_compilation = mp4tag->compilation() == MP4::Tag::Undefined ? MP4GetMetadataCompilation(handle, &compilation) : false;

    char *tool = NULL;
    MP4GetMetadataTool(handle, &tool);

    MP4MetadataDelete(handle);
#endif



#define setmeta(val, tag) \
    if(mp4tag->val().isNull()) { \
        /*MP4DeleteMetadata##tag(handle);*/ \
        MP4SetMetadata##tag(handle, ""); \
    } else { \
        MP4SetMetadata##tag(handle, mp4tag->val().toCString(true)); \
    }

    setmeta(title, Name);
    setmeta(artist, Artist);
    setmeta(album, Album);
    setmeta(comment, Comment);
    setmeta(genre, Genre);

    char buf[100] = "";
    if(mp4tag->year())
        snprintf(buf, sizeof(buf), "%u", mp4tag->year());
    MP4SetMetadataYear(handle, buf);
    u_int16_t t1, t2;
    MP4GetMetadataTrack(handle, &t1, &t2);
    MP4SetMetadataTrack(handle, mp4tag->track(), t2);
    if(mp4tag->bpm() != 0)
        MP4SetMetadataTempo(handle, mp4tag->bpm());
    if(mp4tag->compilation() != MP4::Tag::Undefined) {
        MP4SetMetadataCompilation(handle, mp4tag->compilation());
    }

    MP4SetMetadataCoverArt(handle, mp4tag->cover().size() ? const_cast<u_int8_t *>( reinterpret_cast<const u_int8_t *>( mp4tag->cover().data() ) ) : 0, mp4tag->cover().size());

#ifdef MP4V2_HAS_WRITE_BUG
    // set the saved data again

    if(has_compilation)
        MP4SetMetadataCompilation(handle, compilation);
    if(tool)
    {
        MP4SetMetadataTool(handle, tool);
        free(tool);
    }
#endif

    MP4Close(handle);

    mp4file = MP4Read(name());
    if(mp4file == MP4_INVALID_FILE_HANDLE)
    {
        fprintf(stderr, "reopen failed\n");
        return false;
    }

    return true;
}

bool MP4::File::isOpen()
{
    return mp4file != MP4_INVALID_FILE_HANDLE;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void MP4::File::read(bool readProperties, Properties::ReadStyle propertiesStyle)
{
    properties =  new MP4::Properties(propertiesStyle);
    mp4tag = new MP4::Tag();

    if (mp4file != MP4_INVALID_FILE_HANDLE) {

        if(readProperties)
        {
            // Parse bitrate etc.
            properties->readMP4Properties( mp4file );
        }

        mp4tag->readTags( mp4file );
    }
}

}
