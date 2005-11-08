// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef TAGLIB_AUDIBLEFILETYPERESOLVER_H
#define TAGLIB_AUDIBLEFILETYPERESOLVER_H

#include <taglib/tfile.h>
#include <taglib/fileref.h>


class AudibleFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(const char *fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

#endif
