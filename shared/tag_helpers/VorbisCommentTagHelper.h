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


#ifndef VORBISCOMMENTTAGHELPER_H
#define VORBISCOMMENTTAGHELPER_H

#include "TagHelper.h"

#include <taglib/xiphcomment.h>

namespace Meta
{
    namespace Tag
    {
        class AMAROK_EXPORT VorbisCommentTagHelper : public TagHelper
        {
            public:
                VorbisCommentTagHelper( TagLib::Ogg::XiphComment *tag, Amarok::FileType fileType );

                virtual Meta::FieldHash tags() const;
                virtual bool setTags( const Meta::FieldHash &changes );

                virtual TagLib::ByteVector render() const;

            private:
                TagLib::Ogg::XiphComment *m_tag;
        };
    }
}

#endif // VORBISCOMMENTTAGHELPER_H
