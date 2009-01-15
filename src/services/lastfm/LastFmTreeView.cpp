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

#include "LastFmTreeModel.h" //FIXME just for enums
#include "LastFmTreeView.h"

#include "Amarok.h"
#include "Debug.h"
#include "SvgHandler.h"
#include "PopupDropperFactory.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"

#include <KIcon>
#include <KMenu>

#include <QDropEvent>
#include <QHeaderView>
#include <QMap>
#include <QPainter>
#include <QWidgetAction>
#include <QSlider>
#include <QTimer>
#include <QDesktopServices>
#include <QStandardItemModel>

LastFmTreeView::LastFmTreeView ( QWidget* parent )
        : Amarok::PrettyTreeView ( parent )
        , m_timer ( 0 )
        , m_appendAction ( 0 )
        , m_loadAction ( 0 )
{
//     connect ( this, SIGNAL ( activated ( QModelIndex ) ), SLOT ( onActivated ( QModelIndex ) ) );

    header()->hide();
//     setRootIsDecorated( false );
}


LastFmTreeView::~LastFmTreeView()
{
}

void
LastFmTreeView::contextMenuEvent ( QContextMenuEvent* event )
{
    m_currentItems.clear();
    foreach ( QModelIndex i, selectedIndexes() )
    {
        if ( i.isValid() )
            m_currentItems << i;
    }
    if ( m_currentItems.isEmpty() )
        return;
    PopupDropperAction separator ( this );
    separator.setSeparator ( true );
    PopupDropperActionList actions;
    QModelIndex index = currentIndex();
    LastFmTreeItem* i = static_cast<LastFmTreeItem*> ( index.internalPointer() );
    switch ( i->type() )
    {
    case LastFm::MyRecommendations:
    case LastFm::PersonalRadio:
    case LastFm::LovedTracksRadio:
    case LastFm::NeighborhoodRadio:
    case LastFm::FriendsChild:
    case LastFm::NeighborsChild:
    case LastFm::MyTagsChild:
    case LastFm::UserChildLoved:
    case LastFm::UserChildPersonal:
    case LastFm::UserChildNeighborhood:
    {
        if ( m_appendAction == 0 )
        {
            m_appendAction = new PopupDropperAction ( The::svgHandler()->getRenderer ( "amarok/images/pud_items.svg" ), "append", KIcon ( "media-track-add-amarok" ), i18n ( "&Append to Playlist" ), this );
            connect ( m_appendAction, SIGNAL ( triggered() ), this, SLOT ( slotAppendChildTracks() ) );
        }

        actions.append ( m_appendAction );

        if ( m_loadAction == 0 )
        {
            m_loadAction = new PopupDropperAction ( The::svgHandler()->getRenderer ( "amarok/images/pud_items.svg" ), "load", KIcon ( "folder-open" ), i18nc ( "Replace the currently loaded tracks with these", "&Load" ), this );
            connect ( m_loadAction, SIGNAL ( triggered() ), this, SLOT ( slotPlayChildTracks() ) );
        }
        actions.append ( m_loadAction );
    }
    default:
        break;
    }
    actions += &separator;
    KMenu menu;
    foreach ( PopupDropperAction * action, actions )
    menu.addAction ( action );
    menu.exec ( event->globalPos() );
}

void
LastFmTreeView::onActivated ( const QModelIndex& i )
{
//     contextMenuHandler ( i, DoQMenuDefaultAction );
}

void
LastFmTreeView::slotPlayChildTracks()
{
    playChildTracks ( m_currentItems, Playlist::LoadAndPlay );
}

void
LastFmTreeView::slotAppendChildTracks()
{
    playChildTracks ( m_currentItems, Playlist::AppendAndPlay );
}

void
LastFmTreeView::playChildTracks ( const QModelIndexList &items, Playlist::AddOptions insertMode )
{
    debug() << "LASTFM current items : " << items.size();
    Meta::TrackList list;
    foreach ( QModelIndex item, items )
    {
        LastFmTreeItem *i = static_cast<LastFmTreeItem*> ( item.internalPointer() );
        Meta::TrackPtr track ( i->track() );
        if ( track )
            list << track;
    }
    qStableSort ( list.begin(), list.end(), Meta::Track::lessThan );
    The::playlistController()->insertOptioned ( list, insertMode );
}
