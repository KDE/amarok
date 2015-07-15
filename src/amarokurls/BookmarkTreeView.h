/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef BOOKMARKTREEVIEW_H
#define BOOKMARKTREEVIEW_H

#include "amarok_export.h"
#include "AmarokUrl.h"
#include "BookmarkModel.h"
#include "BookmarkViewItem.h"
#include "widgets/PrettyTreeView.h"

#include <QSortFilterProxyModel>

class QMenu;
 
class PopupDropper;
class QAction;

class QAction;

class AMAROK_EXPORT BookmarkTreeView : public QTreeView
{
    Q_OBJECT

public:
    BookmarkTreeView( QWidget *parent = 0 );
    ~BookmarkTreeView();

    void setNewGroupAction( QAction * action );
    QMenu* contextMenu( const QPoint& point );

    void setProxy( QSortFilterProxyModel *proxy );

protected:
    void keyPressEvent( QKeyEvent *event );
    void mouseDoubleClickEvent( QMouseEvent *event );
    void contextMenuEvent( QContextMenuEvent *event );
    void resizeEvent( QResizeEvent *event );
    bool viewportEvent( QEvent *event );

protected Q_SLOTS:
    void slotLoad();
    void slotDelete();
    void slotRename();

    void slotEdit( const QModelIndex &index );

    //for testing...
    void slotCreateTimecodeTrack() const;

    void slotSectionResized( int logicalIndex, int oldSize, int newSize );
    void slotSectionCountChanged( int oldCount, int newCount );

    void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );

Q_SIGNALS:
    void bookmarkSelected( AmarokUrl bookmark );
    void showMenu( QMenu*, const QPointF& );
    
private:
    QSet<BookmarkViewItemPtr> selectedItems() const;
    QList<QAction *> createCommonActions( QModelIndexList indices );

    QAction *m_loadAction;
    QAction *m_deleteAction;

    //for testing...
    QAction *m_createTimecodeTrackAction;

    QAction *m_addGroupAction;

    QMap<BookmarkModel::Column, qreal> m_columnsSize;

    QSortFilterProxyModel * m_proxyModel;
};

#endif
