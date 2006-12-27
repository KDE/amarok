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

#include "mp4audioproperties.h"
#include "mp4propsproxy.h"

using namespace TagLib;

class MP4::AudioProperties::AudioPropertiesPrivate
{
public:
  MP4::Mp4PropsProxy* propsproxy;
}; // AudioPropertiesPrivate

MP4::AudioProperties::AudioProperties():TagLib::AudioProperties(TagLib::AudioProperties::Average)
{
  d = new MP4::AudioProperties::AudioPropertiesPrivate();
}

MP4::AudioProperties::~AudioProperties()
{
  delete d;
}

void MP4::AudioProperties::setProxy( Mp4PropsProxy* proxy )
{
  d->propsproxy = proxy;
}

int MP4::AudioProperties::length() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->seconds();
}

int MP4::AudioProperties::bitrate() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->bitRate()/1000;
}

int MP4::AudioProperties::sampleRate() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->sampleRate();
}

int MP4::AudioProperties::channels() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->channels();
}

