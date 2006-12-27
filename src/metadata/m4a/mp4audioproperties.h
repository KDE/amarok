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

#ifndef MP4AUDIOPROPERTIES_H
#define MP4AUDIOPROPERTIES_H MP4AUDIOPROPERTIES_H

#include "audioproperties.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4PropsProxy;

    class AudioProperties : public TagLib::AudioProperties
    {
    public:
      //! constructor
      AudioProperties();
      //! destructor
      ~AudioProperties();

      //! function to set the proxy
      void setProxy( Mp4PropsProxy* proxy );

      /*!
       * Returns the length of the file in seconds.
       */
      int length() const;

      /*!
       * Returns the most appropriate bit rate for the file in kb/s.  For constant
       * bitrate formats this is simply the bitrate of the file.  For variable
       * bitrate formats this is either the average or nominal bitrate.
       */
      int bitrate() const;

      /*!
       * Returns the sample rate in Hz.
       */
      int sampleRate() const;

      /*!
       * Returns the number of audio channels.
       */
      int channels() const;

    private:
      class AudioPropertiesPrivate;
      AudioPropertiesPrivate* d;
    };
  } // namespace MP4
} // namespace TagLib

#endif // MP4AUDIOPROPERTIES_H
