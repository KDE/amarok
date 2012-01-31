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

#include "widgets/PrettyTreeView.h"

#include <QMutex>

class PopupDropper;
class KAction;
class QKeyEvent;
class QMouseEvent;
class QContextMenuEvent;

namespace PlaylistBrowserNS {

class PlaylistBrowserView : public Amarok::PrettyTreeView
{
Q_OBJECT
public:
    explicit PlaylistBrowserView( QAbstractItemModel *model, QWidget *parent = 0 );
    ~PlaylistBrowserView();

    virtual void setModel( QAbstractItemModel *model );

    void setNewFolderAction( KAction *action );

signals:
    void currentItemChanged( const QModelIndex &current );

protected:
    //TODO:re-implement QWidget::dragEnterEvent() to show drop-not-allowed indicator

    virtual void keyPressEvent( QKeyEvent *event );
    virtual void mousePressEvent( QMouseEvent *event );
    virtual void mouseReleaseEvent( QMouseEvent *event );
    virtual void mouseMoveEvent( QMouseEvent *event );
    virtual void startDrag( Qt::DropActions supportedActions );

    virtual void contextMenuEvent( QContextMenuEvent* event );
    virtual bool viewportEvent( QEvent *event );

protected slots:
    /** reimplemented to emit a signal */
    void currentChanged( const QModelIndex &current, const QModelIndex &previous );

    /** Used for executing default (first) action when Enter/Return/double click. */
    void slotActivated( const QModelIndex &index );

private:
    QAction *decoratorActionAt( const QModelIndex &idx, const QPoint position );
    QList<QAction *> actionsFor( QModelIndexList indexes );

    PopupDropper* m_pd;

    KAction *m_addFolderAction;

    bool m_ongoingDrag;
    QMutex m_dragMutex;
    bool m_expandToggledWhenPressed;
};

} // namespace PlaylistBrowserNS

#endif // PLAYLISTBROWSERVIEW_H
