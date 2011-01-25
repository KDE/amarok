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


#ifndef ASFTAGHELPER_H
#define ASFTAGHELPER_H

#include "TagHelper.h"

#include <taglib/asftag.h>

namespace Meta
{
    namespace Tag
    {
        class AMAROK_EXPORT ASFTagHelper : public TagHelper
        {
            public:
                ASFTagHelper( TagLib::Tag *tag, TagLib::ASF::Tag *asfTag, Amarok::FileType fileType );

                virtual Meta::FieldHash tags() const;
                virtual bool setTags( const Meta::FieldHash &changes );

#ifndef UTILITIES_BUILD
                virtual bool hasEmbeddedCover() const;
                virtual QImage embeddedCover() const;
                virtual bool setEmbeddedCover(const QImage &cover);
#endif  //UTILITIES_BUILD

            private:
                TagLib::ASF::Tag *m_tag;
        };
    }
}

#endif // ASFTAGHELPER_H
