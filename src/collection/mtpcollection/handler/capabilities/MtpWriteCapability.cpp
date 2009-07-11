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

#include "MtpWriteCapability.h"
#include "MtpHandler.h"

using namespace Handler;

MtpWriteCapability::MtpWriteCapability( Meta::MtpHandler *handler )
        : Handler::WriteCapability()
        , m_handler( handler )
{
}

QStringList
MtpWriteCapability::supportedFormats()
{
    return m_handler->supportedFormats();
}

void
MtpWriteCapability::findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack )
{
    m_handler->findPathToCopy( srcTrack, destTrack );
}


bool
MtpWriteCapability::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    return m_handler->libCopyTrack( srcTrack, destTrack );
}


bool
MtpWriteCapability::libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libDeleteTrackFile( track );
}


void
MtpWriteCapability::libCreateTrack( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->libCreateTrack( track );
}


void
MtpWriteCapability::libDeleteTrack( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->libDeleteTrack( track );
}


void
MtpWriteCapability::addTrackInDB( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->addTrackInDB( track );
}


void
MtpWriteCapability::removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track )
{
    m_handler->removeTrackFromDB( track );
}


void
MtpWriteCapability::databaseChanged()
{
    m_handler->databaseChanged();
}


void
MtpWriteCapability::libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title )
{
    m_handler->libSetTitle( track, title );
}


void
MtpWriteCapability::libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album )
{
    m_handler->libSetAlbum( track, album );
}


void
MtpWriteCapability::libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist )
{
    m_handler->libSetArtist( track, artist );
}


void
MtpWriteCapability::libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer )
{
    m_handler->libSetComposer( track, composer );
}


void
MtpWriteCapability::libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre )
{
    m_handler->libSetGenre( track, genre );
}


void
MtpWriteCapability::libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year )
{
    m_handler->libSetYear( track, year );
}


void
MtpWriteCapability::libSetLength( Meta::MediaDeviceTrackPtr &track, int length )
{
    m_handler->libSetLength( track, length );
}


void
MtpWriteCapability::libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum )
{
    m_handler->libSetTrackNumber( track, tracknum );
}


void
MtpWriteCapability::libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment )
{
    m_handler->libSetComment( track, comment );
}


void
MtpWriteCapability::libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum )
{
    m_handler->libSetDiscNumber( track, discnum );
}


void
MtpWriteCapability::libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate )
{
    m_handler->libSetBitrate( track, bitrate );
}


void
MtpWriteCapability::libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate )
{
    m_handler->libSetSamplerate( track, samplerate );
}


void
MtpWriteCapability::libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm )
{
    m_handler->libSetBpm( track, bpm );
}


void
MtpWriteCapability::libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize )
{
    m_handler->libSetFileSize( track, filesize );
}


void
MtpWriteCapability::libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount )
{
    m_handler->libSetPlayCount( track, playcount );
}

void
MtpWriteCapability::libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed )
{
    m_handler->libSetLastPlayed( track, lastplayed );
}

void
MtpWriteCapability::libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )
{
    m_handler->libSetRating( track, rating );
}

void
MtpWriteCapability::libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type )
{
    m_handler->libSetType( track, type );
}

void
MtpWriteCapability::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    m_handler->libSetPlayableUrl( destTrack, srcTrack );
}

void
MtpWriteCapability::prepareToCopy()
{
    m_handler->prepareToCopy();
}


void
MtpWriteCapability::prepareToDelete()
{
    m_handler->prepareToDelete();
}

void
MtpWriteCapability::updateTrack( Meta::MediaDeviceTrackPtr &track )
{
    m_handler->updateTrack( track );
}

#include "MtpWriteCapability.moc"
