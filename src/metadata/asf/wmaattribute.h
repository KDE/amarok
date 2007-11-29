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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef TAGLIB_WMAATTRIBUTE_H
#define TAGLIB_WMAATTRIBUTE_H

#include <tstring.h>
#include <tbytevector.h>

namespace TagLib {

  namespace WMA {
 
    class File;  
      
    class Attribute {

      friend class File;  
        
    public:

      /*!
       * Enum of types an Attribute can have.
       */
      enum AttributeTypes {
        UnicodeType = 0,
        BytesType   = 1,
        BoolType    = 2,
        DWordType   = 3,
        QWordType   = 4,
        WordType    = 5
      };

      /*!
       * Constructs an empty attribute.
       */       
      Attribute();
      
      /*!
       * Constructs an attribute with \a key and \a value.
       */       
      Attribute(const String &key, const String &value);
      Attribute(const String &key, const ByteVector &value);
      Attribute(const String &key, unsigned int value);
      Attribute(const String &key, unsigned long long value);
      Attribute(const String &key, unsigned short value);
      Attribute(const String &key, bool value);
      
      /*!
       * Construct an attribute as a copy of \a item.
       */
      Attribute(const Attribute &item);       
      
      /*!
       * Destroys the attribute.
       */ 
      virtual ~Attribute();

      /*!
       * Returns the name.
       */
      String name() const;
      
      /*!
       * Returns type of the value.
       */
      AttributeTypes type() const;
      
      /*!
       * Returns the value as a String.
       */
      String toString() const;
      
      /*!
       * Returns the value as a ByteVector.
       */
      ByteVector toByteVector() const;
      
      /*!
       * Returns the value as an integer.
       */
      int toInt() const;
      
      /*!
       * Returns the value as a long long.
       */
      long long toLongLong() const;
      
      ByteVector render() const;
      
    protected:
    
      Attribute(WMA::File &file);
      bool parse(WMA::File &file);
      
    private:
      
      class AttributePrivate;
      AttributePrivate *d;
    
    };
  
  }
  
}

#endif
