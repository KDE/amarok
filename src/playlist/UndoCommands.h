/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
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

#ifndef AMAROK_UNDOCOMMANDS_H
#define AMAROK_UNDOCOMMANDS_H

#include "meta/Meta.h"

#include <QList>
#include <QPair>
#include <QUndoCommand>

namespace Playlist
{
typedef QPair<Meta::TrackPtr, int> InsertCmd;
typedef QList<InsertCmd> InsertCmdList;
class InsertTracksCmd : public QUndoCommand
{
public:
    InsertTracksCmd( QUndoCommand* parent, const InsertCmdList& );
    void undo();
    void redo();
private:
    const InsertCmdList m_cmdlist;
};

typedef QPair<Meta::TrackPtr, int> RemoveCmd;
typedef QList<RemoveCmd> RemoveCmdList;
class RemoveTracksCmd: public QUndoCommand
{
public:
    RemoveTracksCmd( QUndoCommand* parent, const RemoveCmdList& );
    void undo();
    void redo();
private:
    const RemoveCmdList m_cmdlist;
};

typedef QPair<int, int> MoveCmd;
typedef QList<MoveCmd> MoveCmdList;
class MoveTracksCmd: public QUndoCommand
{
public:
    MoveTracksCmd( QUndoCommand* parent, const MoveCmdList& );
    void undo();
    void redo();
private:
    const MoveCmdList m_cmdlist;
};
}

#endif
