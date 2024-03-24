/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
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

#ifndef COLLECTIONTREEVIEW_H
#define COLLECTIONTREEVIEW_H

#include "amarok_export.h"
#include "BrowserDefines.h"
#include "widgets/PrettyTreeView.h"
#include "browsers/CollectionTreeItem.h"
#include "core/meta/forward_declarations.h"
#include "playlist/PlaylistController.h"

#include <QModelIndex>
#include <QMutex>
#include <QSet>
#include <QApplication>

class AmarokMimeData;
class CollectionSortFilterProxyModel;
class CollectionTreeItemModelBase;
class PopupDropper;
namespace Collections {
    class Collection;
    class QueryMaker;
}
class QAction;
class QSortFilterProxyModel;

typedef QList<QAction *> QActionList;

class AMAROK_EXPORT CollectionTreeView : public Amarok::PrettyTreeView
{
        Q_OBJECT

    public:
        explicit CollectionTreeView( QWidget *parent = nullptr );
        ~CollectionTreeView() override;

        QSortFilterProxyModel* filterModel() const;

        void setLevels( const QList<CategoryId::CatMenuId> &levels );
        QList<CategoryId::CatMenuId> levels() const;

        void setLevel( int level, CategoryId::CatMenuId type );

        void setModel( QAbstractItemModel *model ) override;

        //Helper function to remove children if their parent is already present
        static QSet<CollectionTreeItem*> cleanItemSet( const QSet<CollectionTreeItem*> &items );
        static bool onlyOneCollection( const QModelIndexList &indices );
        static Collections::Collection *getCollection( const QModelIndex &index );
        Collections::QueryMaker* createMetaQueryFromItems( const QSet<CollectionTreeItem*> &items, bool cleanItems=true ) const;

        /**
         * Copies all selected tracks to the local collection. The user can also
         * choose to do on-the-fly transcoding.
         */
        void copySelectedToLocalCollection();

    public Q_SLOTS:
        void slotSetFilter( const QString &filter );

        /**
         * This should append all currently visible tracks to the playlist. Takes
         * care to ensure that the tracks are added only after any pending searches
         * are finished.
         */
        void slotAddFilteredTracksToPlaylist();

        void playChildTracksSlot( Meta::TrackList list );

    Q_SIGNALS:
        /**
         * This signal is emitted when slotAddFilteredTracksToPlaylist() has done its
         * work.
         */
        void addingFilteredTracksDone();

    protected:
        void contextMenuEvent( QContextMenuEvent *event ) override;
        void mouseDoubleClickEvent( QMouseEvent *event ) override;
        void mouseReleaseEvent( QMouseEvent *event ) override;
        void keyPressEvent( QKeyEvent *event ) override;
        void dragEnterEvent( QDragEnterEvent *event ) override;
        void dragMoveEvent( QDragMoveEvent *event ) override;
        void startDrag( Qt::DropActions supportedActions ) override;

    protected Q_SLOTS:
        void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected ) override;
        void slotCollapsed( const QModelIndex &index );
        void slotExpanded( const QModelIndex &index );
        void slotExpandIndex( const QModelIndex &index );

        void slotCheckAutoExpand( bool reallyExpand = true );
        void slotCheckAutoExpandReally() { slotCheckAutoExpand( true ); }

        void slotReplacePlaylistWithChildTracks();
        void slotAppendChildTracks();
        void slotQueueChildTracks();
        void slotEditTracks();
        void slotCopyTracks();
        void slotMoveTracks();
        void slotTrashTracks();
        void slotDeleteTracks();
        void slotOrganize();

    private:
        // Utility function to play all items
        // that have this as a parent..
        void playChildTracks( CollectionTreeItem *item, Playlist::AddOptions insertMode );
        void playChildTracks( const QSet<CollectionTreeItem*> &items, Playlist::AddOptions insertMode );
        void editTracks( const QSet<CollectionTreeItem*> &items ) const;
        void organizeTracks( const QSet<CollectionTreeItem*> &items ) const;
        void copyTracks( const QSet<CollectionTreeItem*> &items, Collections::Collection *destination,
                         bool removeSources) const;
        void removeTracks( const QSet<CollectionTreeItem*> &items, bool useTrash ) const;

        // creates different actions from the different objects.
        // note: you should not delete the created actions.
        QActionList createBasicActions( const QModelIndexList &indices );
        QActionList createExtendedActions( const QModelIndexList &indices );
        QActionList createCollectionActions( const QModelIndexList &indices );
        QActionList createCustomActions( const QModelIndexList &indices );

        QHash<QAction*, Collections::Collection*> getCopyActions( const QModelIndexList &indices );
        QHash<QAction*, Collections::Collection*> getMoveActions( const QModelIndexList &indices );

        CollectionTreeItem* getItemFromIndex( QModelIndex &index );

        CollectionSortFilterProxyModel *m_filterModel;
        CollectionTreeItemModelBase *m_treeModel;
        PopupDropper* m_pd;
        QAction* m_appendAction;
        QAction* m_loadAction;
        QAction* m_editAction;
        QAction* m_organizeAction;
        QAction* m_collapseAction;

        QHash<QAction*, Collections::Collection*> m_currentCopyDestination;
        QHash<QAction*, Collections::Collection*> m_currentMoveDestination;

        QMap<AmarokMimeData*, Playlist::AddOptions> m_playChildTracksMode;

        QSet<CollectionTreeItem*> m_currentItems;

        bool m_ongoingDrag;

    Q_SIGNALS:
        void itemSelected( CollectionTreeItem * item );
        void leavingTree();
};

#endif
