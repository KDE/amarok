/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef ID3V2TAGHELPER_H
#define ID3V2TAGHELPER_H

#include "TagHelper.h"

#include <taglib/id3v2tag.h>

namespace Meta
{
    namespace Tag
    {
        class AMAROK_EXPORT ID3v2TagHelper : public TagHelper
        {
            public:
                ID3v2TagHelper( TagLib::Tag *tag, TagLib::ID3v2::Tag *id3v2Tag, Amarok::FileType fileType );

                virtual Meta::FieldHash tags() const;
                virtual bool setTags( const Meta::FieldHash &changes );

                virtual TagLib::ByteVector render() const;

#ifndef UTILITIES_BUILD
                virtual bool hasEmbeddedCover() const;
                virtual QImage embeddedCover() const;
                virtual bool setEmbeddedCover( const QImage &cover );
#endif  //UTILITIES_BUILD

            private:
                TagLib::ID3v2::Tag *m_tag;
        };
    }
}

#endif // ID3V2TAGHELPER_H
