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

#ifndef BOXFACTORY_H
#define BOXFACTORY_H

#include "taglib.h"
#include "mp4isobox.h"

namespace TagLib
{
  namespace MP4
  {
    class BoxFactory
    {
    public:
      BoxFactory();
      ~BoxFactory();

      //! factory function
      Mp4IsoBox* createInstance( TagLib::File* anyfile, MP4::Fourcc fourcc, uint size, long offset ) const;
    }; // class BoxFactory

  } // namepace MP4
} // namepace TagLib

#endif // BOXFACTORY_H
