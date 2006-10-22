// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef TAGLIB_WAVFILETYPERESOLVER_H
#define TAGLIB_WAVFILETYPERESOLVER_H

#include <taglib/tfile.h>
#include <taglib/fileref.h>


class WavFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(const char *fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

#endif
