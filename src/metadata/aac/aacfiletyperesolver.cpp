// (c) 2006 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "aacfiletyperesolver.h"
#include <taglib/mpegfile.h>

TagLib::File *AACFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && !strcasecmp(ext, ".aac"))
    {
        return new TagLib::MPEG::File(fileName, readProperties, propertiesStyle);
    }

    return 0;
}
