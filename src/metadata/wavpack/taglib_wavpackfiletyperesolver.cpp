// (c) 2006 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "taglib_wavpackfiletyperesolver.h"
#include "wvfile.h"

TagLib::File *WavPackFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && !strcasecmp(ext, ".wv"))
    {
        TagLib::WavPack::File *f = new TagLib::WavPack::File(fileName, readProperties, propertiesStyle);
        if(f->isValid())
            return f;
        else
        {
            delete f;
        }
    }

    return 0;
}
