/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *                                                                         *
 * portions may be (C) 2005 Martin Aumueller <aumuell@reserv.at>           *
 *                                                                         *
 ***************************************************************************/
#ifndef _TAGLIB_REALMEDIAFILETYPERESOLVER_H_
#define _TAGLIB_REALMEDIAFILETYPERESOLVER_H_

#include <taglib/tfile.h>
#include <taglib/fileref.h>


class RealMediaFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
   TagLib::File *createFile(const char *fileName,
                            bool readAudioProperties,
                            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;
};

#endif
