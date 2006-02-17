// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "taglib_mp4filetyperesolver.h"
#include "mp4file.h"

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
