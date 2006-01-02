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
 *             and (C) 2005 by Lukas Lalinsky <lalinsky@gmail.com>         *
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2 or higher as published by the Free Software Foundation.             *
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
 ***************************************************************************/

#include <tfile.h>
#include <audioproperties.h>
#include <tstring.h>
#include <id3v1tag.h>
#include "rmff.h"
#include "taglib_realmediafile.h"

using namespace TagLib;
using namespace TagLib::RealMedia;


RealMedia::Tag::Tag(RealMediaFF *rmff, bool allocnew) : m_rmff(rmff), m_owner(allocnew)
{ 
   if (m_owner) 
      m_rmff = new RealMediaFF(*rmff); 
}

RealMedia::Tag::~Tag () 
{ 
   if (m_owner) 
      delete m_rmff; 
}

String RealMedia::Tag::title () const
{
   return m_rmff->title();
}

String RealMedia::Tag::artist () const
{
   return m_rmff->artist();
}

String RealMedia::Tag::album () const
{
   return m_rmff->album();
}

String RealMedia::Tag::comment () const
{
   return m_rmff->comment();
}

String RealMedia::Tag::genre () const
{
   return m_rmff->genre();
}

TagLib::uint RealMedia::Tag::year () const
{
   return m_rmff->year();
}

TagLib::uint RealMedia::Tag::track () const
{
   return m_rmff->track();
}

void   RealMedia::Tag::setTitle (const String &)
{
// TODO: write support
}

void   RealMedia::Tag::setArtist (const String &)
{
// TODO: write support
}

void   RealMedia::Tag::setAlbum (const String &)
{
// TODO: write support
}

void   RealMedia::Tag::setComment (const String &)
{
// TODO: write support
}

void   RealMedia::Tag::setGenre (const String &)
{
// TODO: write support
}

void   RealMedia::Tag::setYear (uint)
{
// TODO: write support
}

void   RealMedia::Tag::setTrack (uint)
{
// TODO: write support
}

bool RealMedia::Tag::isEmpty() const 
{
   return TagLib::Tag::isEmpty() && m_rmff->isEmpty();
}

void RealMedia::Tag::duplicate(const Tag *source, Tag *target, bool overwrite) 
{
   TagLib::Tag::duplicate(source, target, overwrite);
   if (overwrite)
   {
      if (target->m_owner)
      {
         delete target->m_rmff;
         target->m_rmff = new RealMediaFF(*source->m_rmff);
      }
      else
         target->m_rmff = source->m_rmff;
   }
   else
   {
      if (target->isEmpty())
      if (target->m_owner)
      {
         delete target->m_rmff;
         target->m_rmff = new RealMediaFF(*source->m_rmff);
      }
      else
         target->m_rmff = source->m_rmff;
   }
}



int RealMedia::Properties::length () const
{
   return (m_rmff->length() / 1000);
}

int RealMedia::Properties::bitrate () const
{
   return (m_rmff->bitrate() / 1000);
}

int RealMedia::Properties::sampleRate () const
{
   return m_rmff->sampleRate();
}

int RealMedia::Properties::channels () const
{
   return m_rmff->channels();
}


RealMedia::File::File(const char *file, bool readProperties, Properties::ReadStyle propertiesStyle) 
   : TagLib::File(file), m_rmfile(0), m_tag(0), m_props(0)
{
   m_rmfile = new RealMediaFF(file, readProperties, propertiesStyle);
   m_tag = new RealMedia::Tag(m_rmfile);
   m_props = new RealMedia::Properties(m_rmfile);
}

RealMedia::File::~File()
{
   delete m_props;
   delete m_tag;
   delete m_rmfile;
}

TagLib::Tag *RealMedia::File::tag() const
{
  return m_tag;
} 

RealMedia::Tag *RealMedia::File::RealMediaTag() const
{
  return m_tag;
} 

RealMedia::Properties *RealMedia::File::audioProperties() const
{
   return m_props; // m_rmfile->properties;
} 





