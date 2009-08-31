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

#include "UmsWriteCapability.h"
#include "UmsHandler.h"

using namespace Handler;

UmsWriteCapability::UmsWriteCapability( Meta::UmsHandler *handler )
        : Handler::WriteCapabilityBase()
        , m_handler( handler )
{
}

QStringList
UmsWriteCapability::supportedFormats()
{
    return m_handler->supportedFormats();
}

void
UmsWriteCapability::findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack )
{
    m_handler->findPathToCopy( srcTrack, destTrack );
}

bool
UmsWriteCapability::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    return m_handler->libCopyTrack( srcTrack, destTrack );
}

bool
UmsWriteCapability::libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libDeleteTrackFile( track );
}

void
UmsWriteCapability::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    m_handler->libSetPlayableUrl( destTrack, srcTrack );
}

void
UmsWriteCapability::prepareToCopy()
{
    m_handler->prepareToCopy();
}

void
UmsWriteCapability::prepareToDelete()
{
    m_handler->prepareToDelete();
}

void
UmsWriteCapability::updateTrack( Meta::MediaDeviceTrackPtr &track )
{
    m_handler->updateTrack( track );
}

void
UmsWriteCapability::endTrackRemove()
{
    m_handler->endTrackRemove();
}

#include "UmsWriteCapability.moc"
