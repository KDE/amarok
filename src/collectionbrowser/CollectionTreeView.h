/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#ifndef COLLECTIONTREEVIEW_H
#define COLLECTIONTREEVIEW_H

#include "CollectionSortFilterProxyModel.h"
#include "CollectionTreeItem.h"
#include "playlist/PlaylistModel.h"

#include <QSet>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QTreeView>

class QSortFilterProxyModel;
class CollectionSortFilterProxyModel;
class CollectionTreeItemModelBase;
class PopupDropper;
class PopupDropperAction;

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
        const bool showYears() const { return m_showYears; }

        void setShowTrackNumbers( bool show ) { m_showTrackNumbers = show; }
        const bool showTrackNumbers() const { return m_showTrackNumbers; }

    public slots:
        void slotSetFilterTimeout();

        /**
         * Bypass the filter timeout if we really need to start filtering *now*
         */
        void slotFilterNow();

    protected:
        void mousePressEvent( QMouseEvent *event );
        void mouseMoveEvent( QMouseEvent *event );
        void mouseDoubleClickEvent( QMouseEvent *event );

    protected slots:
        virtual void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
        void slotExpand( const QModelIndex &index );
        void slotCollapsed( const QModelIndex &index );

    private:
        // Utility function to play all items
        // that have this as a parent..
        void playChildTracks( CollectionTreeItem *item, Playlist::AddOptions insertMode ) const;
        void playChildTracks( const QSet<CollectionTreeItem*> &items, Playlist::AddOptions insertMode ) const;
        void editTracks( const QSet<CollectionTreeItem*> &items ) const;
        void organizeTracks( const QSet<CollectionTreeItem*> &items ) const;
        PopupDropper* createPopupDropper( QWidget* parent );
        CollectionSortFilterProxyModel *m_filterModel;
        CollectionTreeItemModelBase *m_treeModel;
        QTimer m_filterTimer;
        QPoint m_dragStartPosition;
        bool m_showTrackNumbers;
        bool m_showYears;
        PopupDropper* m_pd;
        PopupDropperAction* m_appendAction;
        PopupDropperAction* m_loadAction;

    signals:
        void itemSelected( CollectionTreeItem * item );
};

#endif
