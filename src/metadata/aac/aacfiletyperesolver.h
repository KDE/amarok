// (c) 2006 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef TAGLIB_AACFILETYPERESOLVER_H
#define TAGLIB_AACFILETYPERESOLVER_H

#include <taglib/tfile.h>
#include <taglib/fileref.h>


class AACFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(const char *fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

#endif
