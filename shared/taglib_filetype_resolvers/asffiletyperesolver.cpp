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
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, see                          *
 *   <http://www.gnu.org/licenses/>.                                       *
 ***************************************************************************/ 

// (c) 2005 Martin Aumueller <aumuell@reserv.at>

#include "asffiletyperesolver.h"
#include "asffile.h"

#include <string.h>

TagLib::File *ASFFileTypeResolver::createFile(TagLib::FileName fileName,
        bool readProperties,
        TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
    if(CheckExtension(fileName, ".wma") || CheckExtension(fileName, ".asf"))
    {
        TagLib::ASF::File *f = new TagLib::ASF::File(fileName, readProperties, propertiesStyle);
        if(f->isValid())
            return f;
        else
        {
            delete f;
        }
    }

    return 0;
}
