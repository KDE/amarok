/***************************************************************************
    copyright            : (C) 2006 by Martin Aumueller
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

#include "wavproperties.h"

#include <taglib/tstring.h>

#include "wavfile.h"

#include <netinet/in.h> // ntohl

using namespace TagLib;

struct WavHeader
{
    uint32_t       riff_id;
    uint32_t       riff_size;
    uint32_t       wave_id;
    uint32_t       format_id;
    uint32_t       format_size;
    uint16_t       format_tag;
    uint16_t       num_channels;
    uint32_t       num_samples_per_sec;
    uint32_t       num_avg_bytes_per_sec;
    uint16_t       num_block_align;
    uint16_t       bits_per_sample;
    uint32_t       data_id;
    uint32_t       num_data_bytes;
};


////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Wav::Properties::Properties(Properties::ReadStyle style) : AudioProperties(style)
{
    m_length = 0;
    m_bitrate = 0;
    m_sampleRate = 0;
    m_channels = 0;
}

Wav::Properties::~Properties()
{
}

int Wav::Properties::length() const
{
    return m_length;
}

int Wav::Properties::bitrate() const
{
    return m_bitrate;
}

int Wav::Properties::sampleRate() const
{
    return m_sampleRate;
}

int Wav::Properties::channels() const
{
    return m_channels;
}

#define swap16(x) ((((x)&0xff00)>>8) | (((x)&0x00ff)<<8))
#define swap32(x) ((swap16((x)&0x0000ffff)<<16) | swap16(((x)&0xffff0000)>>16))

void Wav::Properties::readWavProperties( FILE *fp )
{
    fseek(fp, 0, SEEK_SET );
    WavHeader header;
    if( fread(&header, sizeof(header), 1, fp) != 1 )
    {
        return;
    }

    m_channels = ntohs(swap16(header.num_channels));
    m_sampleRate = ntohl(swap32(header.num_samples_per_sec));
    m_bitrate = ntohl(swap32(header.num_avg_bytes_per_sec)) * 8 / 1000;
    m_length = ntohl(swap32(header.num_data_bytes))/ntohl(swap32(header.num_avg_bytes_per_sec));
}
