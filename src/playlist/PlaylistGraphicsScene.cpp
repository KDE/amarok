/***************************************************************************
 * Copyright 2007  Seb Ruiz <ruiz@kde.org>                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU General Public License as          *
 * published by the Free Software Foundation; either version 2 of          *
 * the License or (at your option) version 3 or any later version          *
 * accepted by the membership of KDE e.V. (or its successor approved       *
 * by the membership of KDE e.V.), which shall act as a proxy              *
 * defined in Section 14 of version 3 of the license.                      *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "PlaylistGraphicsScene.h"

#include "Debug.h"
#include "PlaylistDropVis.h"
#include "PlaylistModel.h"

#include <QGraphicsView>
#include <QMimeData>

#include <typeinfo>


using namespace Playlist;

GraphicsScene::GraphicsScene( QObject* parent )
    : QGraphicsScene( parent ), m_selectionAxis(0)
{
}

void
GraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    QList<QGraphicsItem*> clickedItems = items( event->scenePos() );
    QList<QGraphicsItem*> prevSelected = selectedItems();

    Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool shiftKeyPressed = modifiers & Qt::ShiftModifier;
    const bool controlKeyPressed = modifiers & Qt::ControlModifier;

    if( clickedItems.isEmpty() )
        return QGraphicsScene::mousePressEvent( event );

    if( !(shiftKeyPressed || controlKeyPressed) )
    {
        Playlist::GraphicsItem* axis =
            dynamic_cast<Playlist::GraphicsItem*>( clickedItems.last() );
        if( axis )
        {
            m_selectionAxis = axis;
            connect( axis, SIGNAL(destroyed(QObject*)), SLOT(axisDeleted()) );
        }
        return QGraphicsScene::mousePressEvent( event );
    }


    if( prevSelected.isEmpty() && controlKeyPressed && !shiftKeyPressed )
        return QGraphicsScene::mousePressEvent( event );



    // we assume that playlist items are never overlapping
    QGraphicsItem* clicked = clickedItems.last();

    const bool clickedAlreadySelected = prevSelected.contains( clicked );

    if( shiftKeyPressed )
    {
        QRectF boundingRect;

        QRectF clickedArea = clicked->boundingRect();
        clickedArea.moveTo( clicked->pos() );

        if( m_selectionAxis )
        {
            QRectF axisArea = m_selectionAxis->boundingRect();
            axisArea.moveTo( m_selectionAxis->pos() );

            // is clicked above or below ?
            if( clickedArea.top() >= axisArea.top() )
            {
                boundingRect.setTopLeft( axisArea.topLeft() );
                boundingRect.setBottomRight( clickedArea.bottomRight() );
            }
            else
            {
                boundingRect.setTopLeft( clickedArea.topLeft() );
                boundingRect.setBottomRight( axisArea.bottomRight() );
            }
        }
        else
        {
            boundingRect.setTopLeft( QPointF( 0.0, 0.0 ) );
            boundingRect.setBottomRight( clickedArea.bottomRight() );
        }

        boundingRect.adjust( -1, -1, 1, 1 );

        QPainterPath path;
        path.addRect( boundingRect );
        setSelectionArea( path, Qt::ContainsItemBoundingRect );
    }
    else if( controlKeyPressed )
    {
        QPainterPath path;
        QRectF rect;
        foreach( QGraphicsItem* item, prevSelected )
        {
            if( item == clicked )
                continue;
            rect = item->boundingRect();
            rect.moveTo( item->pos() );
            rect.adjust( 1, 1, -1, -1 );
            path.addRect( rect );
        }

        if( !clickedAlreadySelected )
        {
            rect = clicked->boundingRect();
            rect.moveTo( clicked->pos() );
            rect.adjust( 1, 1, -1, -1 );
            path.addRect( rect );
        }

        setSelectionArea( path, Qt::IntersectsItemBoundingRect );
    }
}

void
GraphicsScene::axisDeleted()
{
    QList<QGraphicsItem*> selected = selectedItems();

    QGraphicsItem* newAxis = 0;
    foreach( QGraphicsItem* item, selected )
    {
        if( item != m_selectionAxis &&
            (newAxis == 0 || item->pos().y() < newAxis->pos().y() ) )
        {
            newAxis = item;
        }
    }

    Playlist::GraphicsItem* axis = dynamic_cast<Playlist::GraphicsItem*>( newAxis );
    if( axis )
    {
        m_selectionAxis = newAxis;
        connect( axis, SIGNAL(destroyed(QObject*)), SLOT(axisDeleted()) );
    }
}



void
GraphicsScene::dragLeaveEvent( QGraphicsSceneDragDropEvent *event )
{
    QGraphicsScene::dragLeaveEvent( event );
}

void
GraphicsScene::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
{
    QGraphicsScene::dragEnterEvent( event );
}

void
GraphicsScene::dropEvent( QGraphicsSceneDragDropEvent *event )
{
    DEBUG_BLOCK
    QGraphicsScene::dropEvent( event );

    if( itemAt( event->pos() ) )
    {
        debug() << "[GS] ignoring";
        event->ignore();
        QGraphicsScene::dropEvent( event );
    }
    else
    {
        debug() << "[GS] accepting drop";
        event->accept();
        The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, -1, 0, QModelIndex() );
        DropVis::instance()->hide();
    }
}

