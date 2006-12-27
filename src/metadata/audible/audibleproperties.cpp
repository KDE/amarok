/***************************************************************************
    copyright            : (C) 2005 by Martin Aumueller
    email                : aumuell@reserv.at

    copyright            : (C) 2005 by Andy Leadbetter
    email                : andrew.leadbetter@gmail.com
                           (original mp4 implementation)
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

#include "audibleproperties.h"

#include <taglib/tstring.h>

#include "taglib_audiblefile.h"

#include <netinet/in.h> // ntohl

using namespace TagLib;


////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Audible::Properties::Properties(Properties::ReadStyle style) : AudioProperties(style)
{
    m_length = 0;
    m_bitrate = 0;
    m_sampleRate = 0;
    m_channels = 0;
}

Audible::Properties::~Properties()
{
}

int Audible::Properties::length() const
{
    return m_length;
}

int Audible::Properties::bitrate() const
{
    return m_bitrate;
}

int Audible::Properties::sampleRate() const
{
    return m_sampleRate;
}

int Audible::Properties::channels() const
{
    return m_channels;
}

#define LENGTH_OFF 61

void Audible::Properties::readAudibleProperties( FILE *fp, int off )
{
    fseek(fp, off+LENGTH_OFF, SEEK_SET );
    fread(&m_length, sizeof(m_length), 1, fp);
    m_length = ntohl(m_length);
    //fprintf(stderr, "len (sec): %d\n", m_length);
    m_bitrate = 0;
    m_sampleRate = 0;
    m_channels = 1;
}
