/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_UNDOCOMMANDS_H
#define AMAROK_UNDOCOMMANDS_H

#include "meta.h"
#include "PlaylistModel.h"


#include <QUndoCommand>

namespace Playlist {
/**
 * AddTracksCmd add tracks to the Playlist::Model. Is a friend of the Playlist::Model.
 * See Qt's QUndoCommand documentation for explanation of the command pattern and such.
 */
    class AddTracksCmd : public QUndoCommand
    {
        public:
            AddTracksCmd( QUndoCommand* parent, int row, Meta::TrackList tracks );
            void undo();
            void redo();
        private:
            Meta::TrackList m_tracks;
            int m_row;
    };
/**
 * Removes specified tracks from the Playlist::Model, and remembers them so as to add them back if requested.
 */
    class RemoveTracksCmd: public QUndoCommand
    {
        public:
            RemoveTracksCmd( QUndoCommand* parent, int position, int numOfRows );
            void undo();
            void redo();
        private:
            int m_numOfRows;
            int m_position;
            Meta::TrackList m_tracks;
    };
}

#endif
