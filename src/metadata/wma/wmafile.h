/***************************************************************************
    copyright            : (C) 2005 by Lukas Lalinsky
    email                : lalinsky@gmail.com
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
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#ifndef TAGLIB_WMAFILE_H
#define TAGLIB_WMAFILE_H

#include <tfile.h>
#include <tag.h>
#include "wmaproperties.h"
#include "wmatag.h"

namespace TagLib {

  namespace WMA {
  
    struct GUID;
    
    typedef unsigned char BYTE;
    typedef unsigned short WORD;
    typedef unsigned int DWORD;
    typedef unsigned long long QWORD;
    
    class File : public TagLib::File
    {
        
      friend class Attribute;

    public:

      File(const char *file, bool readProperties = true, Properties::ReadStyle propertiesStyle = Properties::Average);
      
      virtual ~File();
    
      /*!
       * Returns the TagLib::Tag for this file. 
       */
      virtual TagLib::Tag *tag() const;

      /*!
       * Returns the WMA::Tag for this file. 
       */
      virtual Tag *WMATag() const;

      /*!
       * Returns the WMA::Properties for this file. 
       */
      virtual Properties *audioProperties() const;


      /*!
       * Save the file. 
       *
       * This returns true if the save was successful.
       */
      virtual bool save();
    
    protected:

      int readBYTE();
      int readWORD();
      unsigned int readDWORD();
      long long readQWORD();
      void readGUID(GUID &g);
      void readString(int len, String &s);

      ByteVector renderContentDescription();
      ByteVector renderExtendedContentDescription();
      
      void read(bool readProperties, Properties::ReadStyle propertiesStyle);

    private:
      
      class FilePrivate;
      FilePrivate *d;
      
    };
  
  }

}  

#endif
