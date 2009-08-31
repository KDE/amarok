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

#include "IpodWriteCapability.h"
#include "IpodHandler.h"

using namespace Handler;

IpodWriteCapability::IpodWriteCapability( Meta::IpodHandler *handler )
        : Handler::WriteCapability()
        , m_handler( handler )
{
}

QStringList
IpodWriteCapability::supportedFormats()
{
    return m_handler->supportedFormats();
}

void
IpodWriteCapability::findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack )
{
    m_handler->findPathToCopy( srcTrack, destTrack );
}

bool
IpodWriteCapability::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    return m_handler->libCopyTrack( srcTrack, destTrack );
}

bool
IpodWriteCapability::libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libDeleteTrackFile( track );
}

void
IpodWriteCapability::libCreateTrack( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->libCreateTrack( track );
}

void
IpodWriteCapability::libDeleteTrack( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->libDeleteTrack( track );
}

void
IpodWriteCapability::addTrackInDB( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->addTrackInDB( track );
}

void
IpodWriteCapability::removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->removeTrackFromDB( track );
}

void
IpodWriteCapability::setDatabaseChanged()
{
    m_handler->setDatabaseChanged();
}

void
IpodWriteCapability::libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title )
{
    m_handler->libSetTitle( track, title );
}

void
IpodWriteCapability::libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album )
{
    m_handler->libSetAlbum( track, album );
}

void
IpodWriteCapability::libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist )
{
    m_handler->libSetArtist( track, artist );
}

void
IpodWriteCapability::libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer )
{
    m_handler->libSetComposer( track, composer );
}

void
IpodWriteCapability::libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre )
{
    m_handler->libSetGenre( track, genre );
}

void
IpodWriteCapability::libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year )
{
    m_handler->libSetYear( track, year );
}

void
IpodWriteCapability::libSetLength( Meta::MediaDeviceTrackPtr &track, int length )
{
    m_handler->libSetLength( track, length );
}

void
IpodWriteCapability::libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum )
{
    m_handler->libSetTrackNumber( track, tracknum );
}

void
IpodWriteCapability::libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment )
{
    m_handler->libSetComment( track, comment );
}

void
IpodWriteCapability::libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum )
{
    m_handler->libSetDiscNumber( track, discnum );
}

void
IpodWriteCapability::libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate )
{
    m_handler->libSetBitrate( track, bitrate );
}

void
IpodWriteCapability::libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate )
{
    m_handler->libSetSamplerate( track, samplerate );
}

void
IpodWriteCapability::libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm )
{
    m_handler->libSetBpm( track, bpm );
}

void
IpodWriteCapability::libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize )
{
    m_handler->libSetFileSize( track, filesize );
}

void
IpodWriteCapability::libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount )
{
    m_handler->libSetPlayCount( track, playcount );
}

void
IpodWriteCapability::libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed )
{
    m_handler->libSetLastPlayed( track, lastplayed );
}

void
IpodWriteCapability::libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )
{
    m_handler->libSetRating( track, rating );
}

void
IpodWriteCapability::libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type )
{
    m_handler->libSetType( track, type );
}

void
IpodWriteCapability::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    m_handler->libSetPlayableUrl( destTrack, srcTrack );
}

void
IpodWriteCapability::libSetCoverArt( Meta::MediaDeviceTrackPtr &track, const QPixmap &cover )
{
    m_handler->libSetCoverArt( track, cover );
}

void
IpodWriteCapability::prepareToCopy()
{
    m_handler->prepareToCopy();
}

void
IpodWriteCapability::prepareToDelete()
{
    m_handler->prepareToDelete();
}

#include "IpodWriteCapability.moc"
