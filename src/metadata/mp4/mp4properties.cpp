/***************************************************************************
copyright            : (C) 2005 by Andy Leadbetter
email                : andrew.leadbetter@gmail.com
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

#include "mp4properties.h"


#include <tstring.h>

#include <config.h>
#ifdef HAVE_SYSTEMS_H
#include <systems.h>
#endif

#include <stdint.h>

#ifndef UINT64_TO_DOUBLE
#define UINT64_TO_DOUBLE(a) ((double)((int64_t)(a)))
#endif

using namespace TagLib;


////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MP4::Properties::Properties(Properties::ReadStyle style) : AudioProperties(style)
{
    m_length = 0;
    m_bitrate = 0;
    m_sampleRate = 0;
    m_channels = 0;
}

MP4::Properties::~Properties()
{
}

int MP4::Properties::length() const
{
    return m_length;
}

int MP4::Properties::bitrate() const
{
    return m_bitrate;
}

int MP4::Properties::sampleRate() const
{
    return m_sampleRate;
}

int MP4::Properties::channels() const
{
    return m_channels;
}

void MP4::Properties::readMP4Properties( MP4FileHandle mp4File )
{
    u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

    for (u_int32_t i = 0; i < numTracks; i++)
    {
        MP4TrackId trackId = MP4FindTrackId(mp4File, i);

        const char* trackType =
            MP4GetTrackType(mp4File, trackId);

        if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
        {
            // OK we found an audio track so
            // decode it.
            readAudioTrackProperties(mp4File, trackId );
        }
    }
}

void MP4::Properties::readAudioTrackProperties(MP4FileHandle mp4File,  MP4TrackId trackId )
{

    u_int32_t timeScale =
        MP4GetTrackTimeScale(mp4File, trackId);

    MP4Duration trackDuration =
        MP4GetTrackDuration(mp4File, trackId);

    double msDuration =
        UINT64_TO_DOUBLE(MP4ConvertFromTrackDuration(mp4File, trackId,
                    trackDuration, MP4_MSECS_TIME_SCALE));

    u_int32_t avgBitRate =
        MP4GetTrackBitRate(mp4File, trackId);

    m_bitrate = (avgBitRate + 500) / 1000;
    m_sampleRate = timeScale;
    m_length = (int)(msDuration / 1000.0);
    m_channels = 2;



}
