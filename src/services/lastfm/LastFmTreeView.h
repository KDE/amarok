/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2005-2007 Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>          *
 * Copyright (c) 2005-2007 Max Howell, Last.fm Ltd <max@last.fm>                        *
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

#ifndef LASTFMTREEVIEW_H
#define LASTFMTREEVIEW_H

#include "LastFmTreeModel.h"
#include "playlist/PlaylistModelStack.h"
#include "widgets/PrettyTreeView.h"

#include <QList>
#include <QMutex>
#include <QPointer>

class QContextMenuEvent;
class QMouseEvent;

class PopupDropper;
class QAction;

typedef QList<QAction *> QActionList;

class LastFmTreeView : public Amarok::PrettyTreeView
{
    Q_OBJECT

public:
    LastFmTreeView ( QWidget* parent = 0 );
    ~LastFmTreeView();

signals:
    void statusMessage ( const QString& message );
    void plsShowRestState();
    void plsShowNowPlaying();

private slots:
    void slotPlayChildTracks();
    void slotAppendChildTracks();

protected:
    virtual void contextMenuEvent ( QContextMenuEvent* );
    void mouseDoubleClickEvent( QMouseEvent *event );
    void startDrag( Qt::DropActions supportedActions );

private:
    enum ContextMenuActionType { ExecQMenu, DoQMenuDefaultAction };
    void playChildTracks ( const QModelIndex &item, Playlist::AddOptions insertMode );
    void playChildTracks ( const QModelIndexList &items, Playlist::AddOptions insertMode );
    QActionList createBasicActions( const QModelIndexList &indcies );

    QTimer* m_timer;
    LastFmTreeModel* m_model;
    PopupDropper* m_pd;
    QAction* m_appendAction;
    QAction* m_loadAction;
    QModelIndexList m_currentItems;
    QMutex m_dragMutex;
    bool m_ongoingDrag;
};

#endif
