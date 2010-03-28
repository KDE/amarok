/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009,2010 Téo Mrnjavac <teo@getamarok.com>                             *
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

#include "core/meta/Meta.h"
#include "core/playlists/Playlist.h"
#include "UndoCommands.h"

#include <QObject>

class QUndoStack;

namespace Collections {
    class QueryMaker;
}

namespace Playlist
{
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
    Controller( AbstractModel* bottomModel, AbstractModel* topModel, QObject* parent = 0 );
    ~Controller();

public slots:
    /**
     * Handles the insertion of one single track into the playlist, considering a set of
     * options that handle the specifics of the operation.
     * @param track the track to be inserted.
     * @param options the set of options to be applied to the operation.
     * @see enum AddOptions.
     */
    void insertOptioned( Meta::TrackPtr track, int options );

    /**
     * Handles the insertion of one or more tracks into the playlist, considering a set of
     * options that handle the specifics of the operation.
     * @param list the list of tracks to be inserted.
     * @param options the set of options to be applied to the operation.
     * @see enum AddOptions.
     */
    void insertOptioned( Meta::TrackList list, int options );
    void insertOptioned( Playlists::PlaylistPtr playlist, int options );
    void insertOptioned( Playlists::PlaylistList list, int options );
    void insertOptioned( Collections::QueryMaker *qm, int options );
    void insertOptioned( QList<KUrl>& urls, int options );

    /**
     * Handles the insertion of one or more tracks into the playlist on a specific row.
     * The rows are always considered as topmost playlist model rows.
     * @param topModelRow the insertion row in the topmost model.
     * @param track the track to be inserted.
     */
    void insertTrack( int topModelRow, Meta::TrackPtr track );
    void insertTracks( int topModelRow, Meta::TrackList list );
    void insertPlaylist( int topModelRow, Playlists::PlaylistPtr playlist );
    void insertPlaylists( int topModelRow, Playlists::PlaylistList playlists );
    void insertTracks( int topModelRow, Collections::QueryMaker *qm );
    void insertUrls( int topModelRow, const QList<KUrl>& urls );

    /**
     * Handles the removal of a single track from the playlist.
     * The rows are considered as topmost playlist model rows.
     * @param topModelRow the row to remove in the topmost model.
     */
    void removeRow( int topModelRow );

    /**
     * Handles the removal of tracks from the playlist.
     * The rows are considered as topmost playlist model rows.
     * @param topModelRow the row to remove in the topmost model.
     * @param count the number of rows to remove.
     */
    void removeRows( int topModelRow, int count );

    /**
     * Handles the removal of a list of tracks from the playlist.
     * The rows are considered as topmost playlist model rows.
     * @param topModelRows the list of row numbers to remove.
     */
    void removeRows( QList<int>& topModelRows );

    /**
     * Removes unplayable and duplicate entries in the topmost playlist model, i.e.
     * respects playlist filters.
     */
    void removeDeadAndDuplicates();

    /**
     * Moves a track from one row to another in the playlist.
     * @param from the row containing the track that is about to be moved.
     * @param to the target row where the track should be moved.
     */
    void moveRow( int topModelFrom, int topModelTo );

    /**
     * Moves a list of tracks to a specified row in the playlist.
     * This function returns the real starting location where the rows ended up.
     * For example, if you start with the following playlist:
     *   0 Alpha
     *   1 Bravo
     *   2 Charlie
     *   3 Delta
     *   4 Echo
     *   5 Foxtrot
     * and you call moveRows( [0,1,2], 4 ) then the playlist will end up with
     *   0 Delta
     *   1 Echo
     *   2 Alpha
     *   3 Bravo
     *   4 Charlie
     *   5 Foxtrot
     * and the function will return 2, because that's where the rows really ended up.
     * @param from the list of rows containing the tracks that are about to be moved.
     * @param to the target row where the tracks should be moved.
     * @return the first row where the tracks ended up in the new list.
     */
    int  moveRows( QList<int>& topModelFrom, int topModelTo );
    void moveRows( QList<int>& topModelFrom, QList<int>& topModelTo );

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
    /**
     * Converts a row number in 'm_topModel' to a row in 'm_bottomModel', for purposes of
     * insert. This is not useful for remove/move.
     */
    int insertionTopRowToBottom( int topModelRow );

    /**
     * Handles the insertion of a list of tracks into the playlist on a specific row.
     * The row number is always in the *bottom* playlist model, in contrast to most other
     * functions in this class.
     * @param bottomModelRow the insertion row in the bottom model.
     * @param tl the Meta::TrackList to be inserted.
     */
    void insertionHelper( int bottomModelRow, Meta::TrackList& tl );

    AbstractModel* m_topModel;
    AbstractModel* m_bottomModel;

    QUndoStack* m_undoStack;

    QHash<Collections::QueryMaker*, int> m_queryMap;         //! maps queries to the row where the results should be inserted
    QHash<Collections::QueryMaker*, int> m_optionedQueryMap; //! maps queries to the options to be used when inserting the result
    QHash<Collections::QueryMaker*, Meta::TrackList> m_queryMakerTrackResults;
};
}

#endif
