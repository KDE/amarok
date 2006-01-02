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
#ifndef _TAGLIB_REALMEDIAFILE_H_
#define _TAGLIB_REALMEDIAFILE_H_

#include <tfile.h>
#include <audioproperties.h>
#include <tag.h>

#include <iostream>

class RealMediaFF;
namespace TagLib {

   namespace RealMedia {

      class Tag : public TagLib::Tag
      {
      public:
         Tag(RealMediaFF *rmff, bool allocnew = false);
         virtual   ~Tag ();
         virtual String   title () const;
         virtual String   artist () const;
         virtual String   album () const;
         virtual String   comment () const;
         virtual String   genre () const;
         virtual uint   year () const;
         virtual uint   track () const;
         virtual void   setTitle (const String &s);
         virtual void   setArtist (const String &s);
         virtual void   setAlbum (const String &s);
         virtual void   setComment (const String &s);
         virtual void   setGenre (const String &s);
         virtual void   setYear (uint i);
         virtual void   setTrack (uint i);

         bool isEmpty() const;
         void duplicate(const Tag *source, Tag *target, bool overwrite); 

      private:
         Tag();
         RealMediaFF *m_rmff;
         bool m_owner;
      };


      class Properties : public TagLib::AudioProperties
      {
      public:
         Properties(RealMediaFF *rmff) : TagLib::AudioProperties(Average), m_rmff(rmff) {}
         virtual ~Properties() {}  // you dont own rmff
         virtual int length () const;
         virtual int bitrate () const;
         virtual int sampleRate () const;
         virtual int channels () const;

      private:
         Properties();
         RealMediaFF *m_rmff;
      };
  
      class File : public TagLib::File
      {
      public:

         File(const char *file, bool readProperties = true, Properties::ReadStyle propertiesStyle = Properties::Average);
      
         virtual ~File();
    
         /*
          * Returns the TagLib::Tag for this file. 
          */
         virtual TagLib::Tag *tag() const;
         
         /*
          * Returns the RealMedia::RealMediaTag for this file. 
          */
         virtual Tag *RealMediaTag() const;
         
         /*
          * Returns the RealMedia::Properties for this file. 
          */
         virtual Properties *audioProperties() const;


         /*
          * Save the file. 
          *
          * This returns true if the save was successful.
          */
         virtual bool save() { return false; } // for now
    
      private:
      
         RealMediaFF *m_rmfile;
         Tag         *m_tag;
         Properties  *m_props;
      };
      
   }

}  

#endif
