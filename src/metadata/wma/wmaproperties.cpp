/***************************************************************************
copyright            : (C) 2005 by Umesh Shankar
email                : ushankar@cs.berkeley.edu
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

#include "wmaproperties.h"
#include "wmafile.h"

using namespace TagLib;


////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

WMA::Properties::Properties(Properties::ReadStyle style) : AudioProperties(style)
{
    m_length = 0;
    m_bitrate = 0;
    m_sampleRate = 0;
    m_channels = 0;
}

WMA::Properties::~Properties()
{
}

int WMA::Properties::length() const
{
    return m_length;
}

int WMA::Properties::bitrate() const
{
    return m_bitrate;
}

int WMA::Properties::sampleRate() const
{
    return m_sampleRate;
}

int WMA::Properties::channels() const
{
    return m_channels;
}
