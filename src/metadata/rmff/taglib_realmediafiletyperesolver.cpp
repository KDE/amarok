/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *                                                                         *
 * portions (C) 2005 Martin Aumueller <aumuell@reserv.at>                  *
 *                                                                         *
 ***************************************************************************/
#include <tfile.h>
#include <audioproperties.h>
#include <id3v1tag.h>
#include "taglib_realmediafiletyperesolver.h"
#include "taglib_realmediafile.h"
#include "rmff.h"

TagLib::File *RealMediaFileTypeResolver::createFile(const char *fileName,
                                                    bool readProperties,
                                                    TagLib::AudioProperties::ReadStyle propertiesStyle) const
{
   const char *ext = strrchr(fileName, '.');
   if(ext && (!strcasecmp(ext, ".ra") || !strcasecmp(ext, ".rv") || !strcasecmp(ext, ".rm")))
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
