// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "taglib_audiblefiletyperesolver.h"
#include "taglib_audiblefile.h"

TagLib::File *AudibleFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && !strcasecmp(ext, ".aa"))
    {
        FILE *fp = fopen(fileName, "rb");
        if(!fp)
            return 0;

        return new TagLib::Audible::File(fileName, readProperties, propertiesStyle, fp);
    }

    return 0;
}
