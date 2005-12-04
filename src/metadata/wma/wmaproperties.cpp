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

#include <tstring.h>
#include <wmaproperties.h>
  
using namespace TagLib;

class WMA::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate(): length(0), bitrate(0), sampleRate(0), channels(0) {}
  int length;
  int bitrate;
  int sampleRate;
  int channels;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

WMA::Properties::Properties() : AudioProperties(AudioProperties::Average)
{
  d = new PropertiesPrivate;
}

WMA::Properties::~Properties()
{
  if(d)
    delete d;  
}

int WMA::Properties::length() const
{
  return d->length;
}

int WMA::Properties::bitrate() const
{
  return d->bitrate;
}

int WMA::Properties::sampleRate() const
{
  return d->sampleRate;
}

int WMA::Properties::channels() const
{
  return d->channels;
} 

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void WMA::Properties::set(int length, int bitrate, int sampleRate, int channels)
{
  d->length = length;
  d->bitrate = bitrate;
  d->sampleRate = sampleRate;
  d->channels = channels;
}

