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

#ifndef TAGLIB_WMAPROPERTIES_H
#define TAGLIB_WMAPROPERTIES_H

#include <audioproperties.h>
#include <tstring.h>

namespace TagLib {
    
  namespace WMA {
        
    class File;  
      
    class Properties : public AudioProperties
    {
        
      friend class WMA::File;
      
    public:
    
      /*!
       * Create an instance of WMA::Properties.
       */
      Properties();
      
      /*!
       * Destroys this WMA::Properties instance.
       */
      virtual ~Properties();

      // Reimplementations.

      virtual int length() const;
      virtual int bitrate() const;
      virtual int sampleRate() const;
      virtual int channels() const;

      private:
    
      class PropertiesPrivate;
      PropertiesPrivate *d;
      
      void set(int length, int bitrate, int sampleRate, int channels);
      
    };

  }

}

#endif 
