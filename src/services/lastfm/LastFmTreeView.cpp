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
#include "services/lastfm/LastFmTreeModel.h"

#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QHeaderView>
#include <QMenu>

#include <KLocalizedString>

#include <algorithm>


LastFmTreeView::LastFmTreeView ( QWidget* parent )
        : Amarok::PrettyTreeView ( parent )
        , m_pd( nullptr )
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
    for( const QModelIndex &i : selectedIndexes() )
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
    for( QAction * action : actions )
        menu.addAction ( action );

    menu.exec ( event->globalPos() );
}

QActionList LastFmTreeView::createBasicActions( const QModelIndexList & indices )
{
    Q_UNUSED( indices )
    QActionList actions;
    // last.fm radio was discontinued in 2014, no relevant functionality anymore here
    return actions;
}

void LastFmTreeView::mouseDoubleClickEvent( QMouseEvent *event )
{
    QModelIndex index;
    index = indexAt( event->pos() );

    if( index.isValid() && index.internalPointer() )
    {
        QString url = model()->data(index, LastFm::UrlRole).toString();
        if( url.length() > 18 ) //at least protocol + domain - seems valid
            QDesktopServices::openUrl( QUrl( url ) );
    }
    Q_UNUSED( event )
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
LastFmTreeView::slotAppendChildTracks()
{
}
