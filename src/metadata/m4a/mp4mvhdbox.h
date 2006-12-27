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

#ifndef MP4MVHDBOX_H
#define MP4MVHDBOX_H

#include "mp4isofullbox.h"
#include "mp4fourcc.h"
#include "mp4file.h" // ulonglong

namespace TagLib
{
  namespace MP4
  {
    class Mp4MvhdBox: public Mp4IsoFullBox
    {
    public:
      Mp4MvhdBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4MvhdBox();

      //! function to get the creation time of the mp4 file
      ulonglong creationTime() const;
      //! function to get the modification time of the mp4 file
      ulonglong modificationTime() const;
      //! function to get the timescale referenced by the above timestamps
      uint timescale() const;
      //! function to get the presentation duration in the mp4 file
      ulonglong duration() const;
      //! function to get the rate (playout speed) - typically 1.0;
      uint rate() const;
      //! function to get volume level for presentation - typically 1.0;
      uint volume() const;
      //! function to get the track ID for adding new tracks - useless for this lib
      uint nextTrackID() const;

      //! parse mvhd contents
      void parse();

    private:
      class Mp4MvhdBoxPrivate;
      Mp4MvhdBoxPrivate* d;
    }; // Mp4MvhdBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4MVHDBOX_H
