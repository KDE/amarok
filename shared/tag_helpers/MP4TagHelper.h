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


#ifndef MP4TAGHELPER_H
#define MP4TAGHELPER_H

#include "TagHelper.h"

#include <mp4tag.h>

namespace Meta
{
    namespace Tag
    {
        class AMAROKSHARED_EXPORT MP4TagHelper : public TagHelper
        {
            public:
                MP4TagHelper( TagLib::Tag *tag, TagLib::MP4::Tag *mp4Tag, Amarok::FileType fileType );

                Meta::FieldHash tags() const override;
                bool setTags( const Meta::FieldHash &changes ) override;

                bool hasEmbeddedCover() const override;
                QImage embeddedCover() const override;
                bool setEmbeddedCover( const QImage &cover ) override;

            private:
                TagLib::MP4::Tag *m_tag;
        };
    }
}

#endif // MP4TAGHELPER_H
