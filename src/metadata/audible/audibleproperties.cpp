// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

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
