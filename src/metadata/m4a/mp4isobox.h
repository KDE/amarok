/***************************************************************************
    copyright            : (C) 2002, 2003, 2006 by Jochen Issing
    email                : jochen.issing@isign-softart.de
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

#ifndef MP4ISOBOX_H
#define MP4ISOBOX_H

#include "taglib.h"
#include "mp4fourcc.h"

namespace TagLib
{
  class File; 

  namespace MP4
  {
    class Mp4IsoBox
    {
    public:
      //! constructor for base class
      Mp4IsoBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      //! destructor - simply freeing private ptr
      virtual ~Mp4IsoBox();

      //! function to get the fourcc code
      MP4::Fourcc fourcc() const;
      //! function to get the size of tha atom/box
      uint size() const;
      //! function to get the offset of the atom in the mp4 file
      long offset() const;

      //! parse wrapper to get common interface for both box and fullbox
      virtual void  parsebox();
      //! pure virtual function for all subclasses to implement
      virtual void parse() = 0;

    protected:
      //! function to get the file pointer
      TagLib::File* file() const;

    protected:
      class Mp4IsoBoxPrivate;
      Mp4IsoBoxPrivate* d;
    };

  } // namespace MP4
} // namespace TagLib

#endif // MP4ISOBOX_H

