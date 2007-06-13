/***************************************************************************
      copyright            : (C) 2005 by Paul Cifarelli
      email                : paulc2@optonline.net
 ***************************************************************************/

/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

/***************************************************************************
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

#include <tfile.h>
#include <audioproperties.h>
#include <id3v1tag.h>
#include "taglib_realmediafiletyperesolver.h"
#include "taglib_realmediafile.h"
#include "rmff.h"

#include <string.h>

TagLib::File *RealMediaFileTypeResolver::createFile(const char *fileName,
                                                    bool readProperties,
                                                    TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
   const char *ext = strrchr(fileName, '.');
   if(ext && (!strcasecmp(ext, ".ra") || !strcasecmp(ext, ".rv") || !strcasecmp(ext, ".rm") || 
         !strcasecmp(ext, ".rmj") || !strcasecmp(ext, ".rmvb") ))
   {
      TagLib::RealMedia::File *f = new TagLib::RealMedia::File(fileName, readProperties, propertiesStyle);
      if(f->isValid())
         return f;
      else
      {
         delete f;
      }
   }

   return 0;
}
