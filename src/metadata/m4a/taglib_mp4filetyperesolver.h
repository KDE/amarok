// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef TAGLIB_MP4FILETYPERESOLVER_H
#define TAGLIB_MP4FILETYPERESOLVER_H

#include <taglib/tfile.h>
#include <taglib/fileref.h>


class MP4FileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(const char *fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

#endif
