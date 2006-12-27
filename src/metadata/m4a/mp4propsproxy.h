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

#ifndef MP4PROPSPROXY_H
#define MP4PROPSPROXY_H MP4PROPSPROXY_H
#include "mp4mvhdbox.h"
#include "mp4audiosampleentry.h"

namespace TagLib
{
  namespace MP4
  {
    //! Mp4PropsProxy is used to access the stsd box and mvhd box directly
    /*! this class works as a shortcut to avoid stepping through all parent boxes
     *  to access the boxes in question
     */
    class Mp4PropsProxy
    {
    public:
      //! constructor for properties proxy
      Mp4PropsProxy();
      //! destructor
      ~Mp4PropsProxy();

      //! function to get length of media in seconds
      TagLib::uint seconds() const;
      //! function to get the nunmber of channels
      TagLib::uint channels() const;
      //! function to get the sample rate
      TagLib::uint sampleRate() const;
      //! function to get the bitrate rate
      TagLib::uint bitRate() const;

      //! function to register the movie header box - mvhd
      void registerMvhd( MP4::Mp4MvhdBox* mvhdbox );
      //! function to register the sample description box
      void registerAudioSampleEntry( MP4::Mp4AudioSampleEntry* audiosampleentry );

    private:
      class Mp4PropsProxyPrivate;
      Mp4PropsProxyPrivate* d;
      
    }; // Mp4PropsProxy
  } // MP4
} // TagLib

#endif // MP4PROPSPROXY_H
