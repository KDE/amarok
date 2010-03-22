/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MtpReadCapability.h"
#include "MtpHandler.h"

using namespace Handler;


MtpReadCapability::MtpReadCapability( Meta::MtpHandler *handler )
    : Handler::ReadCapability()
    , m_handler( handler )
{}

void
MtpReadCapability::prepareToParseTracks()
{
    if( m_handler )
        m_handler->prepareToParseTracks();
}

bool
MtpReadCapability::isEndOfParseTracksList()
{
    return m_handler->isEndOfParseTracksList();
}

void
MtpReadCapability::prepareToParseNextTrack()
{
    if( m_handler )
        m_handler->prepareToParseNextTrack();
}

void
MtpReadCapability::nextTrackToParse()
{
    if( m_handler )
        m_handler->nextTrackToParse();
}

void
MtpReadCapability::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    if( m_handler )
        m_handler->setAssociateTrack( track );
}

QString
MtpReadCapability::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetTitle( track );
}

QString
MtpReadCapability::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetAlbum( track );
}

QString
MtpReadCapability::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetArtist( track );
}

QString
MtpReadCapability::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetComposer( track );
}

QString
MtpReadCapability::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetGenre( track );
}

int
MtpReadCapability::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetYear( track );
}

qint64
MtpReadCapability::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetLength( track );
}

int
MtpReadCapability::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
   return  m_handler->libGetTrackNumber( track );
}

QString
MtpReadCapability::libGetComment( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetComment( track );
}

int
MtpReadCapability::libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track )
{
   return  m_handler->libGetDiscNumber( track );
}

int
MtpReadCapability::libGetBitrate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetBitrate( track );
}

int
MtpReadCapability::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetSamplerate( track );
}

qreal
MtpReadCapability::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetBpm( track );
}

int
MtpReadCapability::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetFileSize( track );
}

int
MtpReadCapability::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetPlayCount( track );
}

uint
MtpReadCapability::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetLastPlayed( track );
}

int
MtpReadCapability::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetRating( track );
}

QString
MtpReadCapability::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetType( track );
}

KUrl
MtpReadCapability::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    return m_handler->libGetPlayableUrl( track );
}

float
MtpReadCapability::usedCapacity() const
{
    return m_handler->usedCapacity();
}

float
MtpReadCapability::totalCapacity() const
{
    return m_handler->totalCapacity();
}

#include "MtpReadCapability.moc"
