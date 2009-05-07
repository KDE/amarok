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
{
}


CollectionBrowserTreeView::~CollectionBrowserTreeView()
{
}

void CollectionBrowserTreeView::mouseDoubleClickEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    
    if( index.isValid() && !KGlobalSettings::singleClick() )
    {
        setExpanded( index, !isExpanded( index ) );
        event->accept();
    }
    else // propagate to base class
        CollectionTreeView::mouseDoubleClickEvent( event );
}

void CollectionBrowserTreeView::mousePressEvent( QMouseEvent *event )
{
    DEBUG_BLOCK
    //If using single click do nothing, because we don't want + to automatically
    //expand anything but rather take care of it in the release event below
    if( KGlobalSettings::singleClick() )
        event->accept();
    else
        CollectionTreeView::mousePressEvent( event );
}

// Reimplement release event to detect a single click.
void CollectionBrowserTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    DEBUG_BLOCK
    QModelIndex index = indexAt( event->pos() );

    if( index.isValid() && KGlobalSettings::singleClick() )
    {
        setExpanded( index, !isExpanded( index ) );
        event->accept();
    }
    else // propagate to base class
        CollectionTreeView::mouseReleaseEvent( event );
}

