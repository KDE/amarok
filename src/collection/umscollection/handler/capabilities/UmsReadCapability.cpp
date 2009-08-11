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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "UmsReadCapability.h"
#include "UmsHandler.h"

using namespace Handler;

UmsReadCapability::UmsReadCapability( Meta::UmsHandler *handler )
        : Handler::ReadCapability()
        , m_handler( handler )
{
}

void
UmsReadCapability::prepareToParseTracks()
{
    m_handler->prepareToParseTracks();
}

bool
UmsReadCapability::isEndOfParseTracksList()
{
    return m_handler->isEndOfParseTracksList();
}

void
UmsReadCapability::prepareToParseNextTrack()
{
    m_handler->prepareToParseNextTrack();
}

void
UmsReadCapability::nextTrackToParse()
{
    m_handler->nextTrackToParse();
}

void
UmsReadCapability::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    m_handler->setAssociateTrack( track );
}

QString
UmsReadCapability::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetTitle( track );
}

QString
UmsReadCapability::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetAlbum( track );
}

QString
UmsReadCapability::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetArtist( track );
}

QString
UmsReadCapability::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetComposer( track );
}

QString
UmsReadCapability::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetGenre( track );
}

int
UmsReadCapability::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetYear( track );
}

int
UmsReadCapability::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetLength( track );
}

int
UmsReadCapability::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
   return  m_handler->libGetTrackNumber( track );
}

QString
UmsReadCapability::libGetComment( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetComment( track );
}

int
UmsReadCapability::libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track )
{
   return  m_handler->libGetDiscNumber( track );
}

int
UmsReadCapability::libGetBitrate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetBitrate( track );
}

int
UmsReadCapability::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetSamplerate( track );
}

float
UmsReadCapability::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetBpm( track );
}

int
UmsReadCapability::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetFileSize( track );
}

int
UmsReadCapability::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetPlayCount( track );
}

uint
UmsReadCapability::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetLastPlayed( track );
}

int
UmsReadCapability::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetRating( track );
}

QString
UmsReadCapability::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetType( track );
}

KUrl
UmsReadCapability::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetPlayableUrl( track );
}

float
UmsReadCapability::usedCapacity() const
{
    return m_handler->usedCapacity();
}

float
UmsReadCapability::totalCapacity() const
{
    return m_handler->totalCapacity();
}

#include "UmsReadCapability.moc"
