/***************************************************************************
    copyright            : (C) 2006 by Martin Aumueller
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

#include "taglib_mp4filetyperesolver.h"
#include "mp4file.h"

#include <string.h>

TagLib::File *MP4FileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
//     fprintf(stderr, "mp4?: %s\n", fileName);
    const char *ext = strrchr(fileName, '.');
    if(ext && (!strcasecmp(ext, ".m4a")
                || !strcasecmp(ext, ".m4b") || !strcasecmp(ext, ".m4p")
                || !strcasecmp(ext, ".mp4")
                || !strcasecmp(ext, ".m4v") || !strcasecmp(ext, ".mp4v")))
    {
        return new TagLib::MP4::File(fileName, readProperties, propertiesStyle);
    }

    return 0;
}
