// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#include "wavfiletyperesolver.h"
#include "wavfile.h"

TagLib::File *WavFileTypeResolver::createFile(const char *fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    const char *ext = strrchr(fileName, '.');
    if(ext && !strcasecmp(ext, ".wav"))
    {
        FILE *fp = fopen(fileName, "rb");
        if(!fp)
            return 0;

        return new TagLib::Wav::File(fileName, readProperties, propertiesStyle, fp);
    }

    return 0;
}
