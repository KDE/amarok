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
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin St, 5th fl, Boston, MA 02110-1301,      *
 *   USA, or check http://www.fsf.org/about/contact.html                   *
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
