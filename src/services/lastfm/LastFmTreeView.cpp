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

#include "LastFmTreeView.h"

#include "PopupDropperFactory.h"
#include "context/ContextView.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "services/lastfm/LastFmTreeModel.h" // FIXME just for enums

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

#include <KLocalizedString>

#include <algorithm>


LastFmTreeView::LastFmTreeView ( QWidget* parent )
        : Amarok::PrettyTreeView ( parent )
        , m_timer ( nullptr )
        , m_pd( nullptr )
        , m_appendAction ( nullptr )
        , m_loadAction ( nullptr )
        , m_dragMutex()
        , m_ongoingDrag( false )
{
//     connect ( this, SIGNAL (activated(QModelIndex)), SLOT (onActivated(QModelIndex)) );

    header()->hide();
//     setRootIsDecorated( false );
}


LastFmTreeView::~LastFmTreeView()
{}

void
LastFmTreeView::contextMenuEvent ( QContextMenuEvent* event )
{
    m_currentItems.clear();
    foreach ( const QModelIndex &i, selectedIndexes() )
    {
        if ( i.isValid() )
            m_currentItems << i;
    }
    if ( m_currentItems.isEmpty() )
        return;
    QAction separator ( this );
    separator.setSeparator ( true );

    QActionList actions = createBasicActions( m_currentItems );

    actions += &separator;
    QMenu menu;
    foreach ( QAction * action, actions )
        menu.addAction ( action );

    menu.exec ( event->globalPos() );
}

QActionList LastFmTreeView::createBasicActions( const QModelIndexList & indices )
{
    Q_UNUSED( indices )
    QActionList actions;
    QModelIndex index = currentIndex();
    QVariant type = model()->data(index, LastFm::TypeRole);
    switch ( type.toInt() )
    {
        case LastFm::MyRecommendations:
        case LastFm::PersonalRadio:
        case LastFm::MixRadio:
        case LastFm::FriendsChild:
        case LastFm::MyTagsChild:
        case LastFm::ArtistsChild:
        case LastFm::UserChildPersonal:
        {
            if ( m_appendAction == nullptr )
            {
                m_appendAction = new QAction ( QIcon::fromTheme( "media-track-add-amarok" ), i18n ( "&Add to Playlist" ), this );
                m_appendAction->setProperty( "popupdropper_svg_id", "append" );
                connect ( m_appendAction, &QAction::triggered, this, &LastFmTreeView::slotAppendChildTracks );
            }

            actions.append ( m_appendAction );

            if ( m_loadAction == nullptr )
            {
                m_loadAction = new QAction ( QIcon::fromTheme( "folder-open" ), i18nc ( "Replace the currently loaded tracks with these", "&Replace Playlist" ), this );
                m_appendAction->setProperty( "popupdropper_svg_id", "load" );
                connect ( m_loadAction, &QAction::triggered, this, &LastFmTreeView::slotReplacePlaylistByChildTracks );
            }
            actions.append ( m_loadAction );
        }
        default:
            break;
    }
    return actions;
}

void LastFmTreeView::mouseDoubleClickEvent( QMouseEvent *event )
{
    QModelIndex index;
    index = indexAt( event->pos() );

    if( index.isValid() && index.internalPointer() )
    {
        playChildTracks( index, Playlist::OnDoubleClickOnSelectedItems );
    }
}

void
LastFmTreeView::startDrag(Qt::DropActions supportedActions)
{
    DEBUG_BLOCK

    //setSelectionMode( QAbstractItemView::NoSelection );

    // When a parent item is dragged, startDrag() is called a bunch of times. Here we prevent that:
    m_dragMutex.lock();
    if( m_ongoingDrag )
    {
        m_dragMutex.unlock();
        return;
    }
    m_ongoingDrag = true;
    m_dragMutex.unlock();

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {

        QModelIndexList indices = selectedIndexes();

        QActionList actions = createBasicActions( indices );

        QFont font;
        font.setPointSize( 16 );
        font.setBold( true );

        for( QAction * action : actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );


        m_currentItems.clear();
        for( const QModelIndex &index : indices )
        {
            if( index.isValid() && index.internalPointer() )
                m_currentItems << index;
        }

        PopupDropperItem* subItem;

        PopupDropper * morePud = nullptr;
        if ( actions.count() > 1 )
        {
            morePud = The::popupDropperFactory()->createPopupDropper( nullptr, true );

            for( QAction * action : actions )
                morePud->addItem( The::popupDropperFactory()->createItem( action ) );
        }
        else
            m_pd->addItem( The::popupDropperFactory()->createItem( actions[0] ) );

        //TODO: Keep bugging i18n team about problems with 3 dots
        if ( actions.count() > 1 )
        {
            subItem = m_pd->addSubmenu( &morePud, i18n( "More..." )  );
            The::popupDropperFactory()->adjustItem( subItem );
        }

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, &PopupDropper::fadeHideFinished, m_pd, &PopupDropper::clear );
        m_pd->hide();
    }

    m_dragMutex.lock();
    m_ongoingDrag = false;
    m_dragMutex.unlock();
}

void
LastFmTreeView::slotReplacePlaylistByChildTracks()
{
    playChildTracks( m_currentItems, Playlist::OnReplacePlaylistAction );
}

void
LastFmTreeView::slotAppendChildTracks()
{
    playChildTracks( m_currentItems, Playlist::OnAppendToPlaylistAction );
}

void
LastFmTreeView::playChildTracks( const QModelIndex &item, Playlist::AddOptions insertMode)
{
    QModelIndexList items;
    items << item;

    playChildTracks( items, insertMode );
}
void
LastFmTreeView::playChildTracks ( const QModelIndexList &items, Playlist::AddOptions insertMode )
{
    debug() << "LASTFM current items : " << items.size();
    Meta::TrackList list;
    foreach ( const QModelIndex &item, items )
    {
        Meta::TrackPtr track = model()->data(item, LastFm::TrackRole).value< Meta::TrackPtr >();
        if ( track )
            list << track;
    }
    std::stable_sort ( list.begin(), list.end(), Meta::Track::lessThan );
    The::playlistController()->insertOptioned( list, insertMode );
}
