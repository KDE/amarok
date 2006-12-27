/***************************************************************************
    copyright            : (C) 2005 by Martin Aumueller
    email                : aumuell@reserv.at
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

// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "taglib_mp4filetyperesolver.h"
#include "mp4file.h"

TagLib::File *MP4FileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && (!strcasecmp(ext, ".m4a")
                || !strcasecmp(ext, ".m4b") || !strcasecmp(ext, ".m4p")
                || !strcasecmp(ext, ".mp4")
                || !strcasecmp(ext, ".m4v") || !strcasecmp(ext, ".mp4v")))
    {
        MP4FileHandle h = MP4Read(fileName, 0);
        if(MP4_INVALID_FILE_HANDLE == h)
        {
            return 0;
        }

        return new TagLib::MP4::File(fileName, readProperties, propertiesStyle, h);
    }

    return 0;
}
