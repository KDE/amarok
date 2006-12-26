// (c) 2006 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef TAGLIB_SPEEXFILETYPERESOLVER_H
#define TAGLIB_SPEEXFILETYPERESOLVER_H

#include <taglib/tfile.h>
#include <taglib/fileref.h>


class SpeexFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(const char *fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

#endif
