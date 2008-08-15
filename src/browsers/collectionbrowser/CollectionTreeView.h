/******************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef COLLECTIONTREEVIEW_H
#define COLLECTIONTREEVIEW_H

#include "CollectionSortFilterProxyModel.h"
#include "CollectionTreeItem.h"
#include "playlist/PlaylistModel.h"
#include "Meta.h"

#include <QSet>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QTreeView>

class QSortFilterProxyModel;
class CollectionSortFilterProxyModel;
class CollectionTreeItemModelBase;
class PopupDropper;
class PopupDropperAction;
class AmarokMimeData;



typedef QList<PopupDropperAction *> PopupDropperActionList;

class CollectionTreeView: public QTreeView
{
        Q_OBJECT

    public:
        CollectionTreeView( QWidget *parent = 0 );
        ~CollectionTreeView();

        QSortFilterProxyModel* filterModel() const;

        AMAROK_EXPORT void setLevels( const QList<int> &levels );
        void setLevel( int level, int type );

        void setModel ( QAbstractItemModel * model );
        void contextMenuEvent(QContextMenuEvent* event);

        void setShowYears( bool show ) { m_showYears = show; }
        bool showYears() const { return m_showYears; }

        void setShowTrackNumbers( bool show ) { m_showTrackNumbers = show; }
        bool showTrackNumbers() const { return m_showTrackNumbers; }

        //Helper function to remove children if their parent is already present
        static QSet<CollectionTreeItem*> cleanItemSet( const QSet<CollectionTreeItem*> &items );

    public slots:
        void slotSetFilterTimeout();

        void playChildTracksSlot( Meta::TrackList list );

        /**
         * Bypass the filter timeout if we really need to start filtering *now*
         */
        void slotFilterNow();

    protected:
        void mousePressEvent( QMouseEvent *event );
        void mouseReleaseEvent( QMouseEvent *event );
        void mouseDoubleClickEvent( QMouseEvent *event );
        void startDrag(Qt::DropActions supportedActions);
        //void changeEvent ( QEvent * event );
        virtual void drawRow ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    protected slots:
        virtual void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
        void slotExpand( const QModelIndex &index );
        void slotCollapsed( const QModelIndex &index );

        void slotPlayChildTracks();
        void slotAppendChildTracks();
        void slotEditTracks();
        void slotCopyTracks();
        void slotMoveTracks();
        void slotOrganize();
        void newPalette( const QPalette & palette );

    private:
        // Utility function to play all items
        // that have this as a parent..
        void playChildTracks( CollectionTreeItem *item, Playlist::AddOptions insertMode );
        void playChildTracks( const QSet<CollectionTreeItem*> &items, Playlist::AddOptions insertMode );
        void editTracks( const QSet<CollectionTreeItem*> &items ) const;
        void organizeTracks( const QSet<CollectionTreeItem*> &items ) const;
        void copyTracks( const QSet<CollectionTreeItem*> &items, Collection *destination, bool removeSources ) const;
        PopupDropperActionList createBasicActions( const QModelIndexList &indcies );
        PopupDropperActionList createExtendedActions( const QModelIndexList &indcies );

        bool onlyOneCollection(  const QModelIndexList &indcies );
        Collection * getCollection( const QModelIndexList &indcies );
        QHash<PopupDropperAction*, Collection*> getCopyActions( const QModelIndexList &indcies );
        QHash<PopupDropperAction*, Collection*> getMoveActions( const QModelIndexList &indcies );

        QueryMaker* createMetaQueryFromItems( const QSet<CollectionTreeItem*> &items, bool cleanItems=true ) const;

        CollectionSortFilterProxyModel *m_filterModel;
        CollectionTreeItemModelBase *m_treeModel;
        QTimer m_filterTimer;
        bool m_showTrackNumbers;
        bool m_showYears;
        PopupDropper* m_pd;
        PopupDropperAction* m_appendAction;
        PopupDropperAction* m_loadAction;
        PopupDropperAction* m_editAction;
        PopupDropperAction* m_organizeAction;

        PopupDropperAction * m_caSeperator;
        PopupDropperAction * m_cmSeperator;


        QHash<PopupDropperAction*, Collection*> m_currentCopyDestination;
        QHash<PopupDropperAction*, Collection*> m_currentMoveDestination;

        QMap<AmarokMimeData*, Playlist::AddOptions> m_playChildTracksMode;

        QSet<CollectionTreeItem*> m_currentItems;

        QMutex m_dragMutex;
        bool m_ongoingDrag;

    signals:
        void itemSelected( CollectionTreeItem * item );
};

#endif
