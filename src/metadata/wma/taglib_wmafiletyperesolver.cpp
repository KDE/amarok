// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "taglib_wmafiletyperesolver.h"
#include "wmafile.h"

TagLib::File *WMAFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && (!strcasecmp(ext, ".wma") || !strcasecmp(ext, ".asf")))
    {
        TagLib::WMA::File *f = new TagLib::WMA::File(fileName, readProperties, propertiesStyle);
        if(f->isValid())
            return f;
        else
        {
            delete f;
        }
    }

    return 0;
}
