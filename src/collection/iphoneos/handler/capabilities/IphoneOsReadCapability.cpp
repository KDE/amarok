/****************************************************************************************
 * Copyright (c) 2009 Martin Aumueller <aumuell@reserv.at>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "IphoneOs::ReadCapability"

#include <kdiskfreespaceinfo.h>

#include "IphoneOsReadCapability.h"
#include "../IphoneOsHandler.h"

namespace Handler
{

IphoneOsReadCapability::IphoneOsReadCapability( Meta::IphoneOsHandler *handler )
: m_handler(handler)
{
    DEBUG_BLOCK
}

bool
IphoneOsReadCapability::isEndOfParseTracksList()
{
    return m_handler->isEndOfParseTracksList();
}

void
IphoneOsReadCapability::prepareToParseTracks()
{
    m_handler->prepareToParseTracks();
}

void
IphoneOsReadCapability::prepareToParseNextTrack()
{
    m_handler->prepareToParseNextTrack();
}

void
IphoneOsReadCapability::nextTrackToParse()
{
    m_handler->nextTrackToParse();
}

void
IphoneOsReadCapability::setAssociateTrack(Meta::MediaDeviceTrackPtr track)
{
    m_handler->setAssociateTrack(track);
}

QString
IphoneOsReadCapability::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->title();
}

QString
IphoneOsReadCapability::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->album()->name();
}

QString
IphoneOsReadCapability::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->artist()->name();
}

QString
IphoneOsReadCapability::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->composer()->name();
}

QString
IphoneOsReadCapability::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->genre()->name();
}

int
IphoneOsReadCapability::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->year()->name().toInt();
}

int
IphoneOsReadCapability::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->length();
}

int
IphoneOsReadCapability::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->trackNumber();
}

QString
IphoneOsReadCapability::libGetComment( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->comment();
}

int
IphoneOsReadCapability::libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->discNumber();
}

int
IphoneOsReadCapability::libGetBitrate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->bitrate();
}

int
IphoneOsReadCapability::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->sampleRate();
}

float
IphoneOsReadCapability::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED(track);

    return 0;
}

int
IphoneOsReadCapability::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->filesize();
}

int
IphoneOsReadCapability::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->playCount();
}

uint
IphoneOsReadCapability::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->lastPlayed();
}

int
IphoneOsReadCapability::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->rating();
}

QString
IphoneOsReadCapability::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->type();
}

KUrl
IphoneOsReadCapability::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->metaForTrack(track)->playableUrl();
}

float
IphoneOsReadCapability::usedCapacity() const
{
    KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo( m_handler->mountPoint() );
    return info.used();
}

float
IphoneOsReadCapability::totalCapacity() const
{
    KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo( m_handler->mountPoint() );
    return info.size();
}

};

#include "IphoneOsReadCapability.moc"
