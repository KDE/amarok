/***************************************************************************
 * Copyright      : (c) 2007  Seb Ruiz <ruiz@kde.org>                      *
 *                  (c) 2008  Daniel Caleb Jones <danielcjones@gmail.com>  *
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

#define DEBUG_PREFIX "Playlist::GraphicsScene"

#include "PlaylistGraphicsScene.h"

#include "Debug.h"
#include "PlaylistDropVis.h"
#include "PlaylistGraphicsView.h"
#include "dialogs/TagDialog.h"
#include "playlist/GroupingProxy.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModel.h"
#include "playlist/view/PlaylistViewCommon.h"

#include <QClipboard>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMimeData>

#include <KApplication>

#include <typeinfo>


Playlist::GraphicsScene::GraphicsScene( GraphicsView* parent )
    : QGraphicsScene( parent )
    , m_playlistView(parent)
    , m_contextMenuItem(0)
{
}

void
Playlist::GraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    QList<QGraphicsItem*> clickedItems = items( event->scenePos() );
    QList<QGraphicsItem*> prevSelected = selectedItems();

    QGraphicsScene::mousePressEvent( event );

    if( clickedItems.isEmpty() )
        return;

    Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool shiftKeyPressed = modifiers & Qt::ShiftModifier;
    const bool controlKeyPressed = modifiers & Qt::ControlModifier;

    // we assume that playlist items are never overlapping
    GraphicsItem* clicked = 
        dynamic_cast<GraphicsItem*>( clickedItems.last() );

    if( !clicked )
        return;

    // are we clicking an album header?
    bool headerClick = false;
    if( clicked->groupMode() == Head ||
        clicked->groupMode() == Head_Collapsed )
    {
        QPointF itemClickPos = clicked->mapFromScene( event->scenePos() );
        QRectF headerRect;
        headerRect.setTopLeft( QPointF( 0.0, 0.0 ) );
        headerRect.setBottom( clicked->albumHeaderHeight() );
        headerRect.setRight( clicked->boundingRect().right() );

        headerClick = headerRect.contains( itemClickPos );
    }


    if( !(shiftKeyPressed || controlKeyPressed) )
    {
        m_selectionAxis = clicked;
        connect( clicked, SIGNAL(destroyed(QObject*)), SLOT(axisDeleted()) );

        m_selectionStack.clear();
        prevSelected.clear();
    }


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

        //TODO: add these newly selected items to the selection stack
    }
    else
    {
        if( headerClick )
        {
            int row = m_playlistView->tracks().indexOf( clicked );
            QModelIndex index = GroupingProxy::instance()->index( row, 0 );
            int span = index.data( GroupedTracksRole ).toInt();

            while( span-- )
                prevSelected.append( m_playlistView->tracks()[row + span] );
        }
        else
            prevSelected.append( clicked );

        QPainterPath path;
        QRectF rect;
        foreach( QGraphicsItem* item, prevSelected )
        {
            if( item == clicked && clickedAlreadySelected && !headerClick )
                continue;
            rect = item->boundingRect();
            rect.moveTo( item->pos() );
            rect.adjust( 1, 1, -1, -1 );
            path.addRect( rect );
            m_selectionStack.push( static_cast<GraphicsItem*>(item) );
        }

        setSelectionArea( path, Qt::IntersectsItemBoundingRect );
    }
}

void
Playlist::GraphicsScene::mouseReleaseEvent( QGraphicsSceneMouseEvent* event )
{
    if( event->button() == Qt::MidButton )
    {
        QList<KUrl> urls;
        urls << KUrl( kapp->clipboard()->text() );
        The::playlistController()->insertOptioned( urls, Append );
    }
    else
    {
        QGraphicsScene::mouseReleaseEvent( event );
    }
}

void
Playlist::GraphicsScene::axisDeleted()
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

    GraphicsItem* axis = dynamic_cast<GraphicsItem*>( newAxis );
    if( axis )
    {
        m_selectionAxis = axis;
        connect( axis, SIGNAL(destroyed(QObject*)), SLOT(axisDeleted()) );
    }
}

void
Playlist::GraphicsScene::contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
    DEBUG_BLOCK
    QGraphicsItem *topItem = itemAt( event->scenePos() );
    if( !topItem )
        return;

    GraphicsItem *item = dynamic_cast<GraphicsItem*>( topItem );
    if( !item )
        item = dynamic_cast<GraphicsItem*>( topItem->parentItem() );
    if( !item ) // we've clicked on empty space
        return;

    item->setSelected( true );
    m_contextMenuItem = item;

    QPointF itemPos = item->mapFromScene(event->scenePos());
    int row = m_playlistView->tracks().indexOf( static_cast<GraphicsItem*>(item) );
    const QModelIndex index = GroupingProxy::instance()->index( row, 0 );
    ViewCommon::trackMenu(m_playlistView, &index, event->screenPos(), item->groupMode() < Body && item->imageLocation().contains( itemPos ));

    event->accept();
}

void
Playlist::GraphicsScene::dropEvent( QGraphicsSceneDragDropEvent *event )
{
    DEBUG_BLOCK
    QGraphicsScene::dropEvent( event );

    if( itemAt( event->pos() ) )
    {
        event->ignore();
        QGraphicsScene::dropEvent( event );
    }
    else
    {
        event->accept();
        The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, -1, 0, QModelIndex() );
        DropVis::instance(m_playlistView)->hide();
    }
}
void
Playlist::GraphicsScene::keyPressEvent( QKeyEvent* event )
{
    const bool moveLine = event->matches( QKeySequence::MoveToNextLine ) ||
                          event->matches( QKeySequence::MoveToPreviousLine );

    const bool selectLine = event->matches( QKeySequence::SelectNextLine ) ||
                            event->matches( QKeySequence::SelectPreviousLine );

    const bool nextLine = event->matches( QKeySequence::MoveToNextLine ) ||
                          event->matches( QKeySequence::SelectNextLine );

    const bool prevLine = event->matches( QKeySequence::MoveToPreviousLine ) ||
                          event->matches( QKeySequence::SelectPreviousLine );

    if( moveLine || selectLine )
    {
        event->accept();

        const int selectedCount = selectedItems().count();
        int row = -1; // default to first item if no item already focused
        GraphicsItem *focused = 0;

        // if we've previously selected an item we need to continue the selection from there
        // we can't rely on selectedItems() since the list order is not specified
        if( !m_selectionStack.isEmpty() && m_selectionStack.top() )
            focused = m_selectionStack.top();
        else if( selectedCount > 0 )
            focused = static_cast<GraphicsItem*>( selectedItems().last() );

        if( focused )
            row = m_playlistView->tracks().indexOf( focused );

        // clear any other selected items if we aren't extending the selection
        if( moveLine )
        {
            clearSelection();
            m_selectionStack.clear();
        }

        // find the new item to select/deselect
        // If we're extending the selection, then we don't want to change the
        // item which we're updating the focus/selection for if we've switched
        // directions.
        if( nextLine )
        {
            if( row == m_playlistView->tracks().size() - 1 )
                row = -1; // loop back to the first item

            focused = m_playlistView->tracks().at( ++row );
        }
        else if( prevLine ) // previous line
        {
            if( row <= 0 )
                row = m_playlistView->tracks().size(); // loop to the last item

            focused = m_playlistView->tracks().at( --row );
        }

        if( focused )
        {
            if( selectLine )
            {
                if( m_selectionStack.contains( focused ) )
                {
                    m_selectionStack.pop()->setSelected( false );
                }
                else
                {
                    m_selectionStack.push( focused );
                    focused->setSelected( true );
                    setFocusItem( focused );
                    focused->ensureVisible();
                }
            }
            else if( moveLine )
            {
                m_selectionStack.push( focused );
                focused->setSelected( true );
                setFocusItem( focused );
                focused->ensureVisible();
            }
        }
        return;
    }
    else if( event->key() == Qt::Key_Return )
    {
        if( !m_selectionStack.isEmpty() && m_selectionStack.top() )
        {
            GraphicsItem *item = m_selectionStack.top();
            if( item )
                The::playlistActions()->play( m_playlistView->tracks().indexOf( item ) );
        }
        event->accept();
        return;
    } else if( event->matches( QKeySequence::Delete ) )
    {
        if( !QGraphicsScene::selectedItems().isEmpty() )
        {
            event->accept();
            m_playlistView->removeSelection();
            return;
        }
    }
    QGraphicsScene::keyPressEvent( event );
}

void
Playlist::GraphicsScene::showItemImage()
{
    if( !m_contextMenuItem )
        return;
    m_contextMenuItem->showImage();
    m_contextMenuItem = 0;
}

void
Playlist::GraphicsScene::fetchItemImage()
{
    if( !m_contextMenuItem )
        return;
    m_contextMenuItem->fetchImage();
    m_contextMenuItem = 0;
}

void
Playlist::GraphicsScene::unsetItemImage()
{
    if( !m_contextMenuItem )
        return;
    m_contextMenuItem->unsetImage();
    m_contextMenuItem = 0;
}

void
Playlist::GraphicsScene::editTrackInformation()
{
    DEBUG_BLOCK
    QList<QGraphicsItem*> selection = QGraphicsScene::selectedItems();

    if( selection.isEmpty() )
        return; // our job here is done.

    Meta::TrackList selected;
    foreach (QGraphicsItem* item, selection) {
        GraphicsItem* pli = dynamic_cast<GraphicsItem*>(item);
        if (pli) {
            selected.append(pli->internalTrack());
            debug() << pli->internalTrack()->prettyName() << "will be edited";
        }
    }
    debug() << "total of" << selected.size() << "tracks to edit";
    TagDialog *dialog = new TagDialog(selected, m_playlistView);
    dialog->show();
}
