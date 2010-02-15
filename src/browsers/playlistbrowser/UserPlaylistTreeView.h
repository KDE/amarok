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

#ifndef USERPLAYLISTTREEVIEW_H
#define USERPLAYLISTTREEVIEW_H

#include "widgets/PrettyTreeView.h"

#include <QMutex>
#include <QTimer>

class PopupDropper;
class QAction;

class KAction;

namespace PlaylistBrowserNS {

class MetaPlaylistModel;

class UserPlaylistTreeView : public Amarok::PrettyTreeView
{
    Q_OBJECT

public:
    explicit UserPlaylistTreeView( QAbstractItemModel *model, QWidget *parent = 0 );
    ~UserPlaylistTreeView();

    void setNewGroupAction( KAction * action );

public slots:
    void createNewGroup();

protected:
    virtual void keyPressEvent( QKeyEvent *event );
    virtual void mousePressEvent( QMouseEvent *event );
    virtual void mouseReleaseEvent( QMouseEvent *event );
    virtual void mouseDoubleClickEvent( QMouseEvent *event );
    virtual void mouseMoveEvent( QMouseEvent *event );
    virtual void startDrag( Qt::DropActions supportedActions );

    virtual void contextMenuEvent( QContextMenuEvent* event );

private slots:
    void slotClickTimeout();

private:
    QAbstractItemModel *m_model;
    PopupDropper* m_pd;

    KAction *m_addGroupAction;

    bool m_ongoingDrag;
    QMutex m_dragMutex;

    QPoint m_clickLocation;
    QTimer m_clickTimer;
    QModelIndex m_savedClickIndex;
    bool m_justDoubleClicked;
};

}
#endif
