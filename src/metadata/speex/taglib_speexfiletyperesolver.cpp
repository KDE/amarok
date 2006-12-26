// (c) 2006 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "taglib_speexfiletyperesolver.h"
#include "speexfile.h"

TagLib::File *SpeexFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && !strcasecmp(ext, ".spx"))
    {
        TagLib::Speex::File *f = new TagLib::Speex::File(fileName, readProperties, propertiesStyle);
        if(f->isValid())
            return f;
        else
        {
            delete f;
        }
    }

    return 0;
}
