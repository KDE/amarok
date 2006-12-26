// (c) 2006 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "taglib_trueaudiofiletyperesolver.h"
#include "ttafile.h"

TagLib::File *TTAFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && !strcasecmp(ext, ".tta"))
    {
        TagLib::TTA::File *f = new TagLib::TTA::File(fileName, readProperties, propertiesStyle);
        if(f->isValid())
            return f;
        else
        {
            delete f;
        }
    }

    return 0;
}
