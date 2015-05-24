/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009,2010 Téo Mrnjavac <teo@kde.org>                                   *
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

#include "UndoCommands.h"
#include "amarok_export.h"
#include "core/meta/forward_declarations.h"
#include "core/playlists/Playlist.h"

#include <QObject>

class QUndoStack;

namespace Playlist
{
class AbstractModel;

/**
 * No options means: append at the end of the playlist (without touching playing
 * state)
 */
enum AddOption
{
    Replace = 1, ///< replaces the playlist instead of default appending (or queueing)
    Queue = 2, ///< inserts media into the queue after the currentTrack instead of default
               ///  appending to the end of the playlist
    PrependToQueue = Queue | 4, ///< prepends media to the queue (after current track), implies Queue
    DirectPlay = PrependToQueue | 8, ///< start playback of the first item in the list, implies PrependToQueue
    RemotePlaylistsAreStreams = 16, ///< treat remote urls pointing to playlists as streams.
                                    ///  only has sense for methods that accept urls or playlists
    StartPlayIfConfigured = 32, ///< start playing the first added track if Amarok is
                                ///  configured so and nothing else is already playing

    // following are "consistency convenience enums" so that it is easy for us to make the
    // bahaviour of similarly-looking UI elements the same. These enums are the preferred
    // ones on calling sites. Feel free to add a new one if you find another UI element
    // that appears on multiple places. Prefix these with On*.
    OnDoubleClickOnSelectedItems = StartPlayIfConfigured,
    OnMiddleClickOnSelectedItems = DirectPlay,
    OnReturnPressedOnSelectedItems = StartPlayIfConfigured, // append, should be kept same as double-click

    OnPlayMediaAction = DirectPlay,
    OnAppendToPlaylistAction = 0, // double-click is always available, so don't add StartPlayIfConfigured here
    OnReplacePlaylistAction = Replace | StartPlayIfConfigured,
    OnQueueToPlaylistAction = Queue | StartPlayIfConfigured,
};
Q_DECLARE_FLAGS( AddOptions, AddOption )

/** The Playlist::Controller allows to add, remove or otherwise change tracks to the playlist.
    Instead of directly talking to The::Playlist or PlaylistModelStack this object
    should be used. It will take care of correctly placing the tracks (even
    if the playlist is sorted) and will handle undo and redo operations.
*/
class AMAROK_EXPORT Controller : public QObject
{
    Q_OBJECT

public:
    /**
     * Accessor for the singleton pattern.
     * @return a pointer to the only instance of Playlist::Controller.
     */
    static Controller *instance();

    /**
     * Singleton destructor.
     */
    static void destroy();

public slots:
    /**
     * Handles the insertion of one single track into the playlist, considering a set of
     * options that handle the specifics of the operation.
     * @param track the track to be inserted.
     * @param options the set of options to be applied to the operation.
     * @see enum AddOptions.
     */
    void insertOptioned( Meta::TrackPtr track, AddOptions options = 0 );

    /**
     * Handles the insertion of one or more tracks into the playlist, considering a set of
     * options that handle the specifics of the operation.
     * @param list the list of tracks to be inserted.
     * @param options the set of options to be applied to the operation.
     * @see enum AddOptions.
     */
    void insertOptioned( Meta::TrackList list, AddOptions options = 0 );
    void insertOptioned( Playlists::PlaylistPtr playlist, AddOptions options = 0 );
    void insertOptioned( Playlists::PlaylistList list, AddOptions options = 0 );
    void insertOptioned( const QUrl &url, AddOptions options = 0 );
    void insertOptioned( QList<QUrl> &urls, AddOptions options = 0 );

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
    void insertUrls( int topModelRow, QList<QUrl> &urls );

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
    int moveRows( QList<int>& topModelFrom, int topModelTo );

    /**
     * Reorders tracks in the playlist. For each i, track at position
     * topModelFrom[i] is moved to the position topModelTo[i]. Note that when track
     * on position A is moved to the position B, the track from position B needs to
     * be moved as well. As a consequence, every track position appearing
     * in topModelFrom needs to appear in topModelTo.
     * @param topModelFrom the list containing positions of tracks to be moved
     * @param topModelTo the list containing positions the tracks should be moved to
     */
    void reorderRows( const QList<int> &topModelFrom, const QList<int> &topModelTo );

    void undo();
    void redo();
    void clear();

signals:
    void canRedoChanged( bool );
    void canUndoChanged( bool );

    void changed();

    void replacingPlaylist();

private slots:
    void slotLoaderWithOptionsFinished( const Meta::TrackList &tracks );
    void slotLoaderWithRowFinished( const Meta::TrackList &tracks );

private:
    Controller();

    ~Controller();

    static Controller *s_instance;       //!< Instance member.

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
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS( Playlist::AddOptions )
Q_DECLARE_METATYPE( Playlist::AddOptions )

namespace The
{
    AMAROK_EXPORT Playlist::Controller* playlistController();
}

#endif
