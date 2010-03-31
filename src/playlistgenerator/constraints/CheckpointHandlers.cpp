/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "Constraint::Checkpoint"

#include "Checkpoint.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"

/******************************
 * Abstract Base Class        *
 ******************************/
ConstraintTypes::Checkpoint::AbstractCheckpointHandler::AbstractCheckpointHandler( const int c )
    : m_checkpointPosition( c )
{
    DEBUG_BLOCK
}

/******************************
 * Track Checkpoint Handler   *
 ******************************/
ConstraintTypes::Checkpoint::TrackCheckpointHandler::TrackCheckpointHandler( const int c, const Meta::TrackPtr& t )
    : Checkpoint::AbstractCheckpointHandler( c )
    , m_trackToMatch( t )
{
    DEBUG_BLOCK
}

int
ConstraintTypes::Checkpoint::TrackCheckpointHandler::find( const Meta::TrackList& ) const
{
    // FIXME
    return -1;
}

bool
ConstraintTypes::Checkpoint::TrackCheckpointHandler::match( const Meta::TrackPtr& ) const
{
    // FIXME
    return true;
}

Meta::TrackPtr
ConstraintTypes::Checkpoint::TrackCheckpointHandler::suggest( const Meta::TrackList& ) const
{
    // FIXME
    return Meta::TrackPtr();
}

/******************************
 * Artist Checkpoint Handler  *
 ******************************/
ConstraintTypes::Checkpoint::ArtistCheckpointHandler::ArtistCheckpointHandler( const int c, const Meta::ArtistPtr& a )
    : Checkpoint::AbstractCheckpointHandler( c )
    , m_artistToMatch( a )
{
    DEBUG_BLOCK
}

int
ConstraintTypes::Checkpoint::ArtistCheckpointHandler::find( const Meta::TrackList& ) const
{
    // FIXME
    return -1;
}

bool
ConstraintTypes::Checkpoint::ArtistCheckpointHandler::match( const Meta::TrackPtr& ) const
{
    // FIXME
    return true;
}

Meta::TrackPtr
ConstraintTypes::Checkpoint::ArtistCheckpointHandler::suggest( const Meta::TrackList& ) const
{
    // FIXME
    return Meta::TrackPtr();
}

/******************************
 * Album Checkpoint Handler   *
 ******************************/
ConstraintTypes::Checkpoint::AlbumCheckpointHandler::AlbumCheckpointHandler( const int c, const Meta::AlbumPtr& l )
    : Checkpoint::AbstractCheckpointHandler( c )
    , m_albumToMatch( l )
{
    DEBUG_BLOCK
}

int
ConstraintTypes::Checkpoint::AlbumCheckpointHandler::find( const Meta::TrackList& ) const
{
    // FIXME
    return -1;
}

bool
ConstraintTypes::Checkpoint::AlbumCheckpointHandler::match( const Meta::TrackPtr& ) const
{
    // FIXME
    return true;
}

Meta::TrackPtr
ConstraintTypes::Checkpoint::AlbumCheckpointHandler::suggest( const Meta::TrackList& ) const
{
    // FIXME
    return Meta::TrackPtr();
}
