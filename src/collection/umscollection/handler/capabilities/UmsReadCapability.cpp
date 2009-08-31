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

#include "UmsReadCapability.h"
#include "UmsHandler.h"

using namespace Handler;

UmsReadCapability::UmsReadCapability( Meta::UmsHandler *handler )
        : Handler::CustomReadCapability()
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

Meta::TrackPtr
UmsReadCapability::sourceTrack()
{
    return m_handler->sourceTrack();
}

#include "UmsReadCapability.moc"
