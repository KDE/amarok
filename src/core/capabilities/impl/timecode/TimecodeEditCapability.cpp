/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "TimecodeEditCapability.h"

#include "core/capabilities/Capability.h"
#include "core/meta/impl/timecode/TimecodeMeta.h"

using namespace Capabilities;

////////////////// TimecodeEditCapability //////////////////

TimecodeEditCapability::TimecodeEditCapability( Meta::TimecodeTrack * track )
{
    m_track = track;
}

void
TimecodeEditCapability::setAlbum( const QString &newAlbum )
{
   m_track->setAlbum( newAlbum );
}

void
TimecodeEditCapability::setArtist( const QString &newArtist )
{
     m_track->setArtist( newArtist );
}

void
TimecodeEditCapability::setComposer( const QString &newComposer )
{
    m_track->setComposer( newComposer );
}

void
TimecodeEditCapability::setGenre( const QString &newGenre )
{
     m_track->setGenre( newGenre );
}

void
TimecodeEditCapability::setYear( const QString &newYear )
{
     m_track->setYear( newYear );
}

void
TimecodeEditCapability::setBpm( const qreal newBpm )
{
     m_track->setBpm( newBpm );
}

void
TimecodeEditCapability::setTitle( const QString &newTitle )
{
     m_track->setTitle( newTitle );
}

void
TimecodeEditCapability::setComment( const QString &newComment )
{
     m_track->setComment( newComment );
}

void
TimecodeEditCapability::setTrackNumber( int newTrackNumber )
{
     m_track->setTrackNumber( newTrackNumber );
}

void
TimecodeEditCapability::setDiscNumber( int newDiscNumber )
{
    m_track->setDiscNumber( newDiscNumber );
}


void
TimecodeEditCapability::beginMetaDataUpdate()
{
    m_track->beginMetaDataUpdate();
}

void
TimecodeEditCapability::endMetaDataUpdate()
{
    m_track->endMetaDataUpdate();
}

void
TimecodeEditCapability::abortMetaDataUpdate()
{
    m_track->abortMetaDataUpdate();
}


