/***************************************************************************
    copyright            : (C) 2005 by Martin Aumueller
    email                : aumuell@reserv.at
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

// (c) 2005 Martin Aumueller <aumuell@reserv.at>

#ifndef TAGLIB_MIMEFILETYPERESOLVER_H
#define TAGLIB_MIMEFILETYPERESOLVER_H

#include <tfile_helper.h>
#include <fileref.h>

class MimeFileTypeResolver : public TagLib::FileRef::FileTypeResolver
{
    TagLib::File *createFile(TagLib::FileName fileName,
            bool readAudioProperties,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle) const;

public:
    virtual ~MimeFileTypeResolver() {}
};

#endif
