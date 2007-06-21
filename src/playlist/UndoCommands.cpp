/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/


#include "TheInstances.h"
#include "UndoCommands.h"

using namespace PlaylistNS;

AddTracksCmd::AddTracksCmd( QUndoCommand* parent, int row, Meta::TrackList tracks )
    : QUndoCommand( i18n("Tracks Added"), parent )
    , m_tracks( tracks )
    , m_row( row )
{  }


void
AddTracksCmd::redo()
{
    The::playlistModel()->insertTracksCommand( m_row, m_tracks );
}

void
AddTracksCmd::undo()
{
    The::playlistModel()->removeRowsCommand( m_row, m_tracks.size() );
}

RemoveTracksCmd::RemoveTracksCmd( QUndoCommand* parent, int position, int numOfRows )
    : QUndoCommand( i18n("Tracks Removed"), parent )
    , m_numOfRows( numOfRows )
    , m_position( position )
{  }

void
RemoveTracksCmd::redo()
{
    m_tracks = The::playlistModel()->removeRowsCommand( m_position, m_numOfRows );
}

void
RemoveTracksCmd::undo()
{
    The::playlistModel()->insertTracksCommand( m_position, m_tracks );
}
