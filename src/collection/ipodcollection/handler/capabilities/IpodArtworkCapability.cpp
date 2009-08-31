/****************************************************************************************
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "IpodArtworkCapability.h"
#include "IpodHandler.h"

using namespace Handler;

IpodArtworkCapability::IpodArtworkCapability( Meta::IpodHandler *handler )
    : ArtworkCapability()
    , m_handler( handler )
{
}

IpodArtworkCapability::~IpodArtworkCapability()
{
    // nothing to do here
}

QPixmap IpodArtworkCapability::getCover( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetCoverArt( track );
}

void IpodArtworkCapability::setCover( Meta::MediaDeviceAlbumPtr album, const QPixmap &pixmap )
{
    foreach( Meta::TrackPtr t, album->tracks() )
    {
        Meta::MediaDeviceTrackPtr track = Meta::MediaDeviceTrackPtr::dynamicCast( t );
        m_handler->libSetCoverArt( track, pixmap );
    }
}

void IpodArtworkCapability::setCoverPath( Meta::MediaDeviceAlbumPtr album, const QString &path )
{
    foreach( Meta::TrackPtr t, album->tracks() )
    {
        Meta::MediaDeviceTrackPtr track = Meta::MediaDeviceTrackPtr::dynamicCast( t );
        m_handler->libSetCoverArtPath( track, path );
    }
}

bool IpodArtworkCapability::canUpdateCover() const
{
    return m_handler->isWritable() && m_handler->supportsArtwork();
}

