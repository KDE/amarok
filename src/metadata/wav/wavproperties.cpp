// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

#include "wavproperties.h"

#include <taglib/tstring.h>

#include "wavfile.h"

#include <netinet/in.h> // ntohl

using namespace TagLib;

struct WavHeader
{
    uint8_t        riff_id[4];
    uint32_t       riff_size;
    uint8_t        wave_id[4];
    uint8_t        format_id[4];
    uint32_t       format_size;
    uint16_t       format_tag;
    uint16_t       num_channels;
    uint32_t       num_samples_per_sec;
    uint32_t       num_avg_bytes_per_sec;
    uint16_t       num_block_align;
    uint16_t       bits_per_sample;
    uint8_t        data_id[4];
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
