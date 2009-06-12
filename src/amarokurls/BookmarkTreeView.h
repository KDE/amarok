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
 
#ifndef BOOKMARKTREEVIEW_H
#define BOOKMARKTREEVIEW_H

#include "amarok_export.h"
#include "AmarokUrl.h"
#include "BookmarkViewItem.h"
#include "widgets/PrettyTreeView.h"

class KMenu;
 
class PopupDropper;
class PopupDropperAction;

class KAction;

class AMAROK_EXPORT BookmarkTreeView : public Amarok::PrettyTreeView
{
    Q_OBJECT

public:
    BookmarkTreeView( QWidget *parent = 0 );
    ~BookmarkTreeView();

    void setNewGroupAction( KAction * action );
    KMenu* contextMenu( const QPoint& point );

protected:
    void keyPressEvent( QKeyEvent *event );
    void mouseDoubleClickEvent( QMouseEvent *event );
    void contextMenuEvent( QContextMenuEvent * event );

protected slots:
    void slotLoad();
    void slotDelete();
    void slotRename();

    //for testing...
    void slotCreateTimecodeTrack();

    void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );

signals:
    void bookmarkSelected( AmarokUrl bookmark );
    void showMenu( KMenu*, const QPointF& );
    
private:
    QSet<BookmarkViewItemPtr> selectedItems() const;
    QList<KAction *> createCommonActions( QModelIndexList indices );

    KAction *m_loadAction;
    KAction *m_deleteAction;
    KAction *m_renameAction;

    //for testing...
    KAction *m_createTimecodeTrackAction;

    KAction *m_addGroupAction;
};

#endif
