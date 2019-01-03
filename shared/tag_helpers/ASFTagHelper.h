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

#include <asftag.h>

namespace Meta
{
    namespace Tag
    {
        class AMAROKSHARED_EXPORT ASFTagHelper : public TagHelper
        {
            public:
                ASFTagHelper( TagLib::Tag *tag, TagLib::ASF::Tag *asfTag, Amarok::FileType fileType );

                Meta::FieldHash tags() const override;
                bool setTags( const Meta::FieldHash &changes ) override;

                bool hasEmbeddedCover() const override;
                QImage embeddedCover() const override;
                bool setEmbeddedCover(const QImage &cover) override;

            private:
                TagLib::ASF::Tag *m_tag;
        };
    }
}

#endif // ASFTAGHELPER_H
