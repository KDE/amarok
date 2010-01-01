/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_PLAYLISTCONTROLLER_H
#define AMAROK_PLAYLISTCONTROLLER_H

#include "meta/Meta.h"
#include "meta/Playlist.h"
#include "UndoCommands.h"
#include "playlist/proxymodels/GroupingProxy.h" //FIXME: this needs to go away
                                                // in favor of a The:: call.

#include <QObject>

class QUndoStack;

namespace Playlist
{
class Controller;
class AbstractModel;

enum AddOptions
{
    Append     = 1,     /// inserts media after the last item in the playlist
    Queue      = 2,     /// inserts media after the currentTrack
    Replace    = 4,     /// clears the playlist first
    DirectPlay = 8,     /// start playback of the first item in the list
    Unique     = 16,    /// don't insert anything already in the playlist
    StartPlay  = 32,    /// start playback of the first item in the list if nothing else playing
    AppendAndPlay = Append | StartPlay,
    LoadAndPlay = Replace | StartPlay,
    AppendAndPlayImmediately = Append | DirectPlay, /// append and start playback of the added item
    LoadAndPlayImmediately = Replace | DirectPlay   /// replace and begin playing of new item
};

class AMAROK_EXPORT Controller : public QObject
{
    Q_OBJECT

public:
    Controller( QObject* parent = 0 );
    ~Controller();

public slots:
    void insertOptioned( Meta::TrackPtr track, int options );
    void insertOptioned( Meta::TrackList list, int options );
    void insertOptioned( Meta::PlaylistPtr playlist, int options );
    void insertOptioned( Meta::PlaylistList list, int options );
    void insertOptioned( QueryMaker *qm, int options );
    void insertOptioned( QList<KUrl>& urls, int options );

    void insertTrack( int row, Meta::TrackPtr track );
    void insertTracks( int row, Meta::TrackList list );
    void insertPlaylist( int row, Meta::PlaylistPtr playlist );
    void insertPlaylists( int row, Meta::PlaylistList playlists );
    void insertTracks( int row, QueryMaker *qm );
    void insertUrls( int row, const QList<KUrl>& urls );

    void removeRow( int row );
    void removeRows( int row, int count );
    void removeRows( QList<int>& rows );

    void removeDeadAndDuplicates(); // Removes unplayable and duplicate entries in the topmost playlist
                                    //  model only (i.e. Respects filtering via "Show only matches" etc)

    void moveRow( int from, int to );
    int  moveRows( QList<int>& from, int to ); // see function definition for info abt return value
    void moveRows( QList<int>& from, QList<int>& to );

    void undo();
    void redo();
    void clear();

signals:
    void canRedoChanged( bool );
    void canUndoChanged( bool );

    void changed();

    void replacingPlaylist();

private slots:
    void newResultReady( const QString&, const Meta::TrackList& );
    void queryDone();
    void slotFinishDirectoryLoader( const Meta::TrackList& );

private:
    void insertionHelper( int row, Meta::TrackList& );

    AbstractModel* m_topmostModel;

    QUndoStack* m_undoStack;

    QHash<QueryMaker*, int> m_queryMap;         //! maps queries to the row where the results should be inserted
    QHash<QueryMaker*, int> m_optionedQueryMap; //! maps queries to the options to be used when inserting the result
    QHash<QueryMaker*, Meta::TrackList> m_queryMakerTrackResults;
};
}

#endif
