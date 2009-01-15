/***************************************************************************
 *   Copyright (c) 2009  Casey Link <unnamedrambler@gmail.com>             *
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef SIDE_BAR_TREE_H
#define SIDE_BAR_TREE_H

#include "widgets/PrettyTreeView.h"
// #include "SideBarRevealPopup.h"
// #include "LastFmMimeData.h"
#include "LastFmTreeModel.h"
// #include "SideBarDelegate.h"
#include "playlist/PlaylistController.h"
#include <QPointer>
#include <QTreeView>
#include <QSet>
#include <QList>
class PopupDropperAction;
typedef QList<PopupDropperAction *> PopupDropperActionList;
class LastFmTreeView : public Amarok::PrettyTreeView
{
    Q_OBJECT

public:
    LastFmTreeView ( QWidget* parent = 0 );
    ~LastFmTreeView();

    //////
//         void addRecentlyPlayedTrack( Track );
//         static QSet<SideBarItem*> cleanItemSet( const QSet<SideBarItem*> &items );
signals:
    void statusMessage ( const QString& message );
    void plsShowRestState();
    void plsShowNowPlaying();

private slots:
//         void expandIndexUnderMouse();
    void onActivated ( const QModelIndex& );
    void slotPlayChildTracks();
    void slotAppendChildTracks();

protected:
    virtual void contextMenuEvent ( QContextMenuEvent* );

private:
    enum ContextMenuActionType { ExecQMenu, DoQMenuDefaultAction };
    void playChildTracks ( const QModelIndexList &items, Playlist::AddOptions insertMode );
    //Helper function to remove children if their parent is already present
//         void dragDropHandler( class QDropEvent* );
//         bool dragDropHandlerPrivate( const QModelIndex&, QDropEvent*, QString& status_message );

    QTimer* m_timer;
    LastFmTreeModel* m_model;
    PopupDropperAction* m_appendAction;
    PopupDropperAction* m_loadAction;
    QModelIndexList m_currentItems;
//         SideBarDelegate* m_delegate;
//         QPointer<RevealPopup> m_revealer;
//         ToolTipLabel* m_drag_tip;
};

#endif
