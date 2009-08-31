/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "IpodReadCapability.h"
#include "IpodHandler.h"

using namespace Handler;

IpodReadCapability::IpodReadCapability( Meta::IpodHandler *handler )
        : Handler::ReadCapability()
        , m_handler( handler )
{
}

void
IpodReadCapability::prepareToParseTracks()
{
    m_handler->prepareToParseTracks();
}

bool
IpodReadCapability::isEndOfParseTracksList()
{
    return m_handler->isEndOfParseTracksList();
}

void
IpodReadCapability::prepareToParseNextTrack()
{
    m_handler->prepareToParseNextTrack();
}

void
IpodReadCapability::nextTrackToParse()
{
    m_handler->nextTrackToParse();
}

void
IpodReadCapability::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    m_handler->setAssociateTrack( track );
}

QString
IpodReadCapability::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetTitle( track );
}

QString
IpodReadCapability::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetAlbum( track );
}

QString
IpodReadCapability::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetArtist( track );
}

QString
IpodReadCapability::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetComposer( track );
}

QString
IpodReadCapability::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetGenre( track );
}

int
IpodReadCapability::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetYear( track );
}

int
IpodReadCapability::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetLength( track );
}

int
IpodReadCapability::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
   return  m_handler->libGetTrackNumber( track );
}

QString
IpodReadCapability::libGetComment( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetComment( track );
}

int
IpodReadCapability::libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track )
{
   return  m_handler->libGetDiscNumber( track );
}

int
IpodReadCapability::libGetBitrate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetBitrate( track );
}

int
IpodReadCapability::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetSamplerate( track );
}

float
IpodReadCapability::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetBpm( track );
}

int
IpodReadCapability::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetFileSize( track );
}

int
IpodReadCapability::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetPlayCount( track );
}

uint
IpodReadCapability::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetLastPlayed( track );
}

int
IpodReadCapability::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetRating( track );
}

QString
IpodReadCapability::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetType( track );
}

KUrl
IpodReadCapability::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetPlayableUrl( track );
}

float
IpodReadCapability::usedCapacity() const
{
    return m_handler->usedCapacity();
}

float
IpodReadCapability::totalCapacity() const
{
    return m_handler->totalCapacity();
}

#include "IpodReadCapability.moc"
