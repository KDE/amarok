/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "UndoCommands.h"

#include "PlaylistModelStack.h"

/************************
 * Insert
 ************************/
Playlist::InsertTracksCmd::InsertTracksCmd( QUndoCommand* parent, const InsertCmdList& cmds )
        : QUndoCommand( i18n( "Tracks Added" ), parent )
        , m_cmdlist( cmds )
{ }

void
Playlist::InsertTracksCmd::redo()
{
    Playlist::ModelStack::instance()->source()->insertTracksCommand( m_cmdlist );
}

void
Playlist::InsertTracksCmd::undo()
{
    Playlist::ModelStack::instance()->source()->removeTracksCommand( m_cmdlist );
}

/************************
 * Remove
 ************************/
Playlist::RemoveTracksCmd::RemoveTracksCmd( QUndoCommand* parent, const RemoveCmdList& cmds )
        : QUndoCommand( i18n( "Tracks Removed" ), parent )
        , m_cmdlist( cmds )
{ }

void
Playlist::RemoveTracksCmd::redo()
{
    Playlist::ModelStack::instance()->source()->removeTracksCommand( m_cmdlist );
}

void
Playlist::RemoveTracksCmd::undo()
{
    Playlist::ModelStack::instance()->source()->insertTracksCommand( m_cmdlist );
}

/************************
 * Move
 ************************/
Playlist::MoveTracksCmd::MoveTracksCmd( QUndoCommand* parent, const MoveCmdList& cmds )
        : QUndoCommand( i18n( "Track moved" ), parent ) // FIXME: better translation after string freeze
        , m_cmdlist( cmds )
{ }

void
Playlist::MoveTracksCmd::redo()
{
    Playlist::ModelStack::instance()->source()->moveTracksCommand( m_cmdlist, false );
}

void
Playlist::MoveTracksCmd::undo()
{
    Playlist::ModelStack::instance()->source()->moveTracksCommand( m_cmdlist, true );
}
