/******************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>          *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                 *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "CollectionBrowserTreeView.h"

#include "Debug.h"

#include <KGlobalSettings>

#include <QMouseEvent>


CollectionBrowserTreeView::CollectionBrowserTreeView( QWidget *parent )
    : CollectionTreeView( parent )
    , m_justDoubleClicked( false )
{
    setMouseTracking( true );
    connect( &m_clickTimer, SIGNAL( timeout() ), this, SLOT( slotClickTimeout() ) );
}

CollectionBrowserTreeView::~CollectionBrowserTreeView()
{
}

void CollectionBrowserTreeView::mouseDoubleClickEvent( QMouseEvent *event )
{
    if( event->button() != Qt::LeftButton || event->modifiers() )
    {
        CollectionTreeView::mouseDoubleClickEvent( event );
        update();
        return;
    }
    m_clickTimer.stop();
    //m_justDoubleClicked is necessary because the mouseReleaseEvent still
    //comes through, but after the mouseDoubleClickEvent, so we need to tell
    //mouseReleaseEvent to ignore that one event
    m_justDoubleClicked = true;

    QModelIndex origIndex = indexAt( event->pos() );
    QModelIndex filteredIndex;
    CollectionTreeView::getIndexForEvent( event, filteredIndex );

    if( filteredIndex.isValid() && !KGlobalSettings::singleClick() )
    {
        CollectionTreeItem *item = static_cast<CollectionTreeItem*>( filteredIndex.internalPointer() );
        if( item->isTrackItem() ) //detect if item is track
        {
            CollectionTreeView::mouseDoubleClickEvent( event );
        }
        else
        {
            setExpanded( origIndex, !isExpanded( origIndex ) );
            event->accept();
        }
        m_clickTimer.stop();
    }
    else // propagate to base class
        CollectionTreeView::mouseDoubleClickEvent( event );
}

void CollectionBrowserTreeView::mousePressEvent( QMouseEvent *event )
{
    if( KGlobalSettings::singleClick() )
        setItemsExpandable( false );
    CollectionTreeView::mousePressEvent( event );
    update();
}

void CollectionBrowserTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    setItemsExpandable( true );
    if( event->button() != Qt::LeftButton
            || event->modifiers()
            || selectedIndexes().size() > 1)
    {
        CollectionTreeView::mouseReleaseEvent( event );
        update();
        return;
    }

    if( m_clickTimer.isActive() || m_justDoubleClicked )
    {
        //it's a double-click...so ignore it
        m_clickTimer.stop();
        m_justDoubleClicked = false;
        m_index = QModelIndex();
        event->accept();
        return;
    }

    QModelIndex index = indexAt( event->pos() );
    m_index = index;
    KConfigGroup cg( KGlobal::config(), "KDE" );
    m_clickTimer.start( cg.readEntry( "DoubleClickInterval", 400 ) );
    m_clickLocation = event->pos();
    event->accept();
}

void CollectionBrowserTreeView::mouseMoveEvent( QMouseEvent *event )
{
    if( event->buttons() || event->modifiers() )
    {
        CollectionTreeView::mouseMoveEvent( event );
        update();
        return;
    }
    QPoint point = event->pos() - m_clickLocation;
    KConfigGroup cg( KGlobal::config(), "KDE" );
    if( point.manhattanLength() > cg.readEntry( "StartDragDistance", 4 ) )
    {
        m_clickTimer.stop();
        slotClickTimeout();
        event->accept();
    }
    else
        CollectionTreeView::mouseMoveEvent( event );
}

void CollectionBrowserTreeView::slotClickTimeout()
{
    m_clickTimer.stop();
    if( m_index.isValid() && KGlobalSettings::singleClick() )
    {
        setExpanded( m_index, !isExpanded( m_index ) );
    }
    m_index = QModelIndex();
}

#include "CollectionBrowserTreeView.moc"
