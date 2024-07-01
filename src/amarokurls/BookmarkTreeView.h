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

class QAction;
class QMenu;


class AMAROK_EXPORT BookmarkTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit BookmarkTreeView( QWidget *parent = nullptr );
    ~BookmarkTreeView() override;

    void setNewGroupAction( QAction * action );
    QMenu* contextMenu( const QPoint& point );

    void setProxy( QSortFilterProxyModel *proxy );
    void slotEdit( const QModelIndex &index );

protected:
    void keyPressEvent( QKeyEvent *event ) override;
    void mouseDoubleClickEvent( QMouseEvent *event ) override;
    void contextMenuEvent( QContextMenuEvent *event ) override;
    void resizeEvent( QResizeEvent *event ) override;
    bool viewportEvent( QEvent *event ) override;

protected Q_SLOTS:
    void slotLoad();
    void slotDelete();
    void slotRename();


    //for testing...
    void slotCreateTimecodeTrack() const;

    void slotSectionResized( int logicalIndex, int oldSize, int newSize );
    void slotSectionCountChanged( int oldCount, int newCount );

    void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected ) override;

Q_SIGNALS:
    void bookmarkSelected( AmarokUrl bookmark );
    void showMenu( QMenu*, const QPointF& );
    
private:
    QSet<BookmarkViewItemPtr> selectedItems() const;
    QList<QAction *> createCommonActions( const QModelIndexList &indices );

    QAction *m_loadAction;
    QAction *m_deleteAction;

    //for testing...
    QAction *m_createTimecodeTrackAction;

    QAction *m_addGroupAction;

    QMap<BookmarkModel::Column, qreal> m_columnsSize;

    QSortFilterProxyModel * m_proxyModel;
};

#endif
