/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef USERPLAYLISTTREEVIEW_H
#define USERPLAYLISTTREEVIEW_H

#include "SqlPlaylistViewItem.h"

#include <qtreeview.h>

class PopupDropper;
class PopupDropperAction;

class KAction;

namespace PlaylistBrowserNS {

/**
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class UserPlaylistTreeView : public QTreeView
{
    Q_OBJECT

public:
    UserPlaylistTreeView( QWidget *parent = 0 );

    ~UserPlaylistTreeView();

    void setNewGroupAction( KAction * action );

protected:
    void keyPressEvent( QKeyEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    void mouseDoubleClickEvent( QMouseEvent *event );
    void startDrag( Qt::DropActions supportedActions );

    void contextMenuEvent( QContextMenuEvent* event );

private slots:
    void slotLoad();
    void slotAppend();
    void slotDelete();
    void slotRename();

private:
    QList<PopupDropperAction *> createCommonActions( QModelIndexList indices );

    PopupDropper* m_pd;

    PopupDropperAction *m_appendAction;
    PopupDropperAction *m_loadAction;

    PopupDropperAction *m_deleteAction;
    PopupDropperAction *m_renameAction;

    KAction *m_addGroupAction;

    QSet<SqlPlaylistViewItemPtr> m_currentItems;

    QPoint m_dragStartPosition;
};

}
#endif
