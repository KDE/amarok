/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef PLAYLISTBROWSERVIEW_H
#define PLAYLISTBROWSERVIEW_H

#include "core/playlists/Playlist.h"
#include "playlist/PlaylistController.h"
#include "widgets/PrettyTreeView.h"

class PopupDropper;
class QKeyEvent;
class QMouseEvent;
class QContextMenuEvent;

namespace PlaylistBrowserNS {

class PlaylistBrowserView : public Amarok::PrettyTreeView
{
Q_OBJECT
public:
    explicit PlaylistBrowserView( QAbstractItemModel *model, QWidget *parent = nullptr );

    void setModel( QAbstractItemModel *model ) override;

Q_SIGNALS:
    void currentItemChanged( const QModelIndex &current );

protected:
    // TODO: re-implement QWidget::dragEnterEvent() to show drop-not-allowed indicator

    void keyPressEvent( QKeyEvent *event ) override;
    void mouseDoubleClickEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void startDrag( Qt::DropActions supportedActions ) override;

    void contextMenuEvent( QContextMenuEvent *event ) override;

protected Q_SLOTS:
    /** reimplemented to Q_EMIT a signal */
    void currentChanged( const QModelIndex &current, const QModelIndex &previous ) override;

private Q_SLOTS:
    // these are connected to m_*Actions:
    void slotCreateEmptyPlaylist();
    void slotAppend();
    void slotLoad();
    void slotSetNew( bool newState );
    void slotRename();
    void slotDelete();
    void slotRemoveTracks();
    void slotExport();

private:
    void insertIntoPlaylist( const QModelIndex &index, Playlist::AddOptions options );
    void insertIntoPlaylist( const QModelIndexList &list, Playlist::AddOptions options );
    void insertIntoPlaylist( Playlist::AddOptions options );

    /**
     * Gets action for a list of indices and sets internal action targets to these.
     *
     * After you have processed/triggered the actions, you should call
     * resetActionTargets() to prevent stale targets laying around.
     */
    QList<QAction *> actionsFor( const QModelIndexList &indexes );
    void resetActionTargets();

    PopupDropper* m_pd;

    QAction *m_createEmptyPlaylistAction;
    QAction *m_appendAction;
    QAction *m_loadAction;
    QAction *m_setNewAction; // for podcasts
    QAction *m_renamePlaylistAction;
    QAction *m_deletePlaylistAction;
    QAction *m_removeTracksAction;
    QAction *m_exportAction;
    QAction *m_separatorAction;
    bool m_ongoingDrag;

    Playlists::PlaylistProvider *m_writableActionProvider;
    Playlists::PlaylistList m_actionPlaylists;
    Playlists::PlaylistList m_writableActionPlaylists;
    QMultiHash<Playlists::PlaylistPtr, int> m_actionTracks; // maps playlists to track positions
    QMultiHash<Playlists::PlaylistPtr, int> m_writableActionTracks;
};

} // namespace PlaylistBrowserNS

#endif // PLAYLISTBROWSERVIEW_H
