/***************************************************************************
 * copyright            : (C) 2008 Soren Harward <stharward@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "Playlist::PrettyListView"

#include "PrettyListView.h"

#include "Debug.h"
#include "PrettyItemDelegate.h"
#include "dialogs/TagDialog.h"
#include "meta/Meta.h"
#include "PaletteHandler.h"
#include "playlist/GroupingProxy.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/view/PlaylistViewCommon.h"

#include <KApplication>

#include <QContextMenuEvent>
#include <QDropEvent>
#include <QItemSelection>
#include <QKeyEvent>
#include <QListView>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPersistentModelIndex>

Playlist::PrettyListView::PrettyListView( QWidget* parent )
        : QListView( parent )
        , m_headerPressIndex( QModelIndex() )
        , m_mousePressInHeader( false )
{
    setModel( GroupingProxy::instance() );
    setItemDelegate( new PrettyItemDelegate( this ) );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDropIndicatorShown( false ); // we draw our own drop indicator
    setAutoScroll( true );

    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    // rendering adjustments
    setFrameShape( QFrame::NoFrame );
    setAlternatingRowColors( true) ;
    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );
    
    setAutoFillBackground( false );

    // signal connections
    connect( this, SIGNAL( doubleClicked( const QModelIndex& ) ), this, SLOT( trackActivated( const QModelIndex& ) ) );
}

Playlist::PrettyListView::~PrettyListView() {}

void
Playlist::PrettyListView::editTrackInformation()
{
    Meta::TrackList tl;
    foreach( const QModelIndex &index, selectedIndexes() )
    {
        tl.append( index.data( TrackRole ).value<Meta::TrackPtr>() );
    }

    if( !tl.isEmpty() )
    {
        TagDialog *dialog = new TagDialog( tl, this );
        dialog->show();
    }
}

void
Playlist::PrettyListView::playTrack()
{
    trackActivated( currentIndex() );
}

void
Playlist::PrettyListView::removeSelection()
{
    QList<int> sr = selectedRows();
    Controller::instance()->removeRows( sr );
    selectionModel()->clearSelection();
}

void
Playlist::PrettyListView::scrollToActiveTrack()
{
    QModelIndex activeIndex = model()->index( GroupingProxy::instance()->activeRow(), 0, QModelIndex() );
    if ( activeIndex.isValid() )
        scrollTo( activeIndex, QAbstractItemView::PositionAtCenter );
}

void
Playlist::PrettyListView::trackActivated( const QModelIndex& idx ) const
{
    Actions::instance()->play( idx );
}

void
Playlist::PrettyListView::contextMenuEvent( QContextMenuEvent* event )
{
    QModelIndex index = indexAt( event->pos() );
    
    if ( !index.isValid() )
        return;
    
    Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool controlKeyPressed = modifiers & Qt::ControlModifier;
    //Ctrl + Right Click is used for quick queue sorting
    if ( controlKeyPressed )
    {
        return;
    }

    ViewCommon::trackMenu( this, &index, event->globalPos(), true );
    event->accept();
}

void
Playlist::PrettyListView::dragLeaveEvent( QDragLeaveEvent* event )
{
    m_mousePressInHeader = false;
    m_dropIndicator = QRect( 0, 0, 0, 0 );
    QListView::dragLeaveEvent( event );
}

void
Playlist::PrettyListView::dragMoveEvent( QDragMoveEvent* event )
{
    QPoint mousept = event->pos() + QPoint( horizontalOffset(), verticalOffset() );
    QModelIndex index = indexAt( event->pos() );
    if ( index.isValid() ) {
        m_dropIndicator = visualRect( index );
    } else {
        // draw it on the bottom of the last item
        index = model()->index( GroupingProxy::instance()->rowCount() - 1, 0, QModelIndex() );
        m_dropIndicator = visualRect( index );
        m_dropIndicator = m_dropIndicator.translated( 0, m_dropIndicator.height() );
    }
    QListView::dragMoveEvent( event );
}

void
Playlist::PrettyListView::dropEvent( QDropEvent* event )
{
    DEBUG_BLOCK
    QRect oldDrop = m_dropIndicator;
    m_dropIndicator = QRect( 0, 0, 0, 0 );
    if ( dynamic_cast<PrettyListView*>( event->source() ) == this )
    {
        QAbstractItemModel* plModel = model();
        int targetRow = indexAt( event->pos() ).row();
        targetRow = ( targetRow < 0 ) ? plModel->rowCount() : targetRow; // target of < 0 means we dropped on the end of the playlist
        QList<int> sr = selectedRows();
        int realtarget = Controller::instance()->moveRows( sr, targetRow );
        QItemSelection selItems;
        foreach( int row, sr )
        {
            Q_UNUSED( row )
            selItems.select( plModel->index( realtarget, 0 ), plModel->index( realtarget, 0 ) );
            realtarget++;
        }
        selectionModel()->select( selItems, QItemSelectionModel::ClearAndSelect );
        event->accept();
    }
    else
    {
        QListView::dropEvent( event );
    }
    // add some padding around the old drop area which to repaint, as we add offsets when painting. See paintEvent().
    oldDrop.adjust( -6, -6, 6, 6 );
    repaint( oldDrop );
}

void
Playlist::PrettyListView::keyPressEvent( QKeyEvent* event )
{
    if ( event->matches( QKeySequence::Delete ) )
    {
        removeSelection();
        event->accept();
    }
    else if ( event->key() == Qt::Key_Return )
    {
        trackActivated( currentIndex() );
        event->accept();
    }
    else if ( event->matches( QKeySequence::SelectAll ) )
    {
        QModelIndex topIndex = model()->index( 0, 0 );
        QModelIndex bottomIndex = model()->index( model()->rowCount() - 1, 0 );
        QItemSelection selItems( topIndex, bottomIndex );
        selectionModel()->select( selItems, QItemSelectionModel::ClearAndSelect );
        event->accept();
    }
    else
    {
        QListView::keyPressEvent( event );
    }
}

void
Playlist::PrettyListView::mousePressEvent( QMouseEvent* event )
{
    if ( mouseEventInHeader( event ) && ( event->button() == Qt::LeftButton ) )
    {
        m_mousePressInHeader = true;
        QModelIndex index = indexAt( event->pos() );
        //only do this dance if header is not already selected
        if ( !selectionModel()->isSelected( index ) ) {
            m_headerPressIndex = QPersistentModelIndex( index );
            int rows = index.data( GroupedTracksRole ).toInt();
            QModelIndex bottomIndex = model()->index( index.row() + rows - 1, 0 );
            QItemSelection selItems( index, bottomIndex );
            QItemSelectionModel::SelectionFlags command = headerPressSelectionCommand( index, event );
            selectionModel()->select( selItems, command );
            // TODO: if you're doing shift-select on rows above the header, then the rows following the header will be lost from the selection
            selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
        }
    }
    else
    {
        m_mousePressInHeader = false;
    }

    //handle Ctrl + Right Click for quick queue sorting
    Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool controlKeyPressed = modifiers & Qt::ControlModifier;
    if ( ( event->button() == Qt::RightButton ) && controlKeyPressed )
    {
        QAbstractItemModel* plModel = model();
        
        //move clicked items to row right after the last selected item.
        int targetRow = selectedRows().isEmpty()
                ? plModel->rowCount()
                : selectedRows().last() + 1;
        selectionModel()->clear();
        
        event->accept();
        
        QModelIndex index = indexAt( event->pos() );
        QList<int> sr;
        if( mouseEventInHeader( event ) )
        {
            int rows = index.data( GroupedTracksRole ).toInt();
            for( int i = index.row(); i < index.row() + rows; i++ )
            {
                sr << i;
            }
        }
        else
        {
            sr << index.row();
        }
        Controller::instance()->moveRows( sr, targetRow );
    }
    else
    {
        QListView::mousePressEvent( event ); //this should always be forwarded, as it is used to determine the offset
        //relative to the mouse of the selection we are dragging!
    }
}

void
Playlist::PrettyListView::mouseReleaseEvent( QMouseEvent* event )
{
    if ( mouseEventInHeader( event ) && ( event->button() == Qt::LeftButton ) && m_mousePressInHeader && m_headerPressIndex.isValid() )
    {
        QModelIndex index = indexAt( event->pos() );
        if ( index == m_headerPressIndex )
        {
            int rows = index.data( GroupedTracksRole ).toInt();
            QModelIndex bottomIndex = model()->index( index.row() + rows - 1, 0 );
            QItemSelection selItems( index, bottomIndex );
            QItemSelectionModel::SelectionFlags command = headerReleaseSelectionCommand( index, event );
            selectionModel()->select( selItems, command );
        }
        event->accept();
    }
    else
    {
        QListView::mouseReleaseEvent( event );
    }
    m_mousePressInHeader = false;
}

bool
Playlist::PrettyListView::mouseEventInHeader( const QMouseEvent* event ) const
{
    QModelIndex index = indexAt( event->pos() );
    if ( index.data( GroupRole ).toInt() == Head )
    {
        QPoint mousePressPos = event->pos();
        mousePressPos.rx() += horizontalOffset();
        mousePressPos.ry() += verticalOffset();
        return PrettyItemDelegate::insideItemHeader( mousePressPos, rectForIndex( index ) );
    }
    else
    {
        return false;
    }
}

void
Playlist::PrettyListView::paintEvent( QPaintEvent* event )
{
    QPoint offset( 6, 0 );
    if ( !m_dropIndicator.size().isEmpty() ) {
        QPalette p = KApplication::palette();
        QPen pen( p.color( QPalette::Highlight ), 6, Qt::SolidLine, Qt::RoundCap );
        QPainter painter( viewport() );
        painter.setPen( pen );
        painter.drawLine( m_dropIndicator.topLeft() + offset, m_dropIndicator.topRight() - offset );
    }
    QListView::paintEvent( event );
}

QItemSelectionModel::SelectionFlags
Playlist::PrettyListView::headerPressSelectionCommand( const QModelIndex& index, const QMouseEvent* event ) const
{
    if ( !index.isValid() )
        return QItemSelectionModel::NoUpdate;

    const bool shiftKeyPressed = event->modifiers() & Qt::ShiftModifier;
    //const bool controlKeyPressed = event->modifiers() & Qt::ControlModifier;
    const bool indexIsSelected = selectionModel()->isSelected( index );
    const bool controlKeyPressed = event->modifiers() & Qt::ControlModifier;
    
    if ( shiftKeyPressed )
        return QItemSelectionModel::SelectCurrent;

    if ( indexIsSelected && controlKeyPressed ) //make this consistent with how single items work. This also makes it possible to drag the header
        return QItemSelectionModel::Deselect;
    
    return QItemSelectionModel::Select;
}

QItemSelectionModel::SelectionFlags
Playlist::PrettyListView::headerReleaseSelectionCommand( const QModelIndex& index, const QMouseEvent* event ) const
{
    if ( !index.isValid() )
        return QItemSelectionModel::NoUpdate;

    const bool shiftKeyPressed = event->modifiers() & Qt::ShiftModifier;
    const bool controlKeyPressed = event->modifiers() & Qt::ControlModifier;

    if ( !controlKeyPressed && !shiftKeyPressed )
        return QItemSelectionModel::ClearAndSelect;
    else
        return QItemSelectionModel::NoUpdate;
}

QList<int>
Playlist::PrettyListView::selectedRows() const
{
    QList<int> rows;
    foreach( const QModelIndex &idx, selectedIndexes() )
    {
        rows.append( idx.row() );
    }
    return rows;
}

void Playlist::PrettyListView::newPalette( const QPalette & palette )
{
    Q_UNUSED( palette )
    The::paletteHandler()->updateItemView( this );
    reset();
}



void Playlist::PrettyListView::find( const QString &searchTerm )
{
    DEBUG_BLOCK
    int row = GroupingProxy::instance()->find( searchTerm );
    if( row != -1 ) {
        //select this track
        debug() << "Got match at row: " << row;
        
        QModelIndex index = model()->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

        QModelIndex foundIndex = model()->index( row, 0, QModelIndex() );
        if ( foundIndex.isValid() )
            scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );

        emit( found() );
    } else {
        emit( notFound() );
    }

    
}

void Playlist::PrettyListView::findNext( const QString & searchTerm )
{
    DEBUG_BLOCK
    QList<int> selected = selectedRows();

    int currentRow = -1;
    if( selected.size() > 0 )
        currentRow = selected.last();

    debug() << "current row is: " << currentRow;
    
    int row = GroupingProxy::instance()->findNext( searchTerm, currentRow );
    if( row != -1 ) {
        //select this track
        debug() << "Got match at row: " << row;
        
        QModelIndex index = model()->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

        QModelIndex foundIndex = model()->index( row, 0, QModelIndex() );
        if ( foundIndex.isValid() )
            scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );
        
        emit( found() );
    } else {
        emit( notFound() );
    }
}

void Playlist::PrettyListView::findPrevious(const QString & searchTerm)
{
    DEBUG_BLOCK
    QList<int> selected = selectedRows();

    int currentRow = GroupingProxy::instance()->totalLength();
    if( selected.size() > 0 )
        currentRow = selected.first();

    debug() << "current row is: " << currentRow;
    
    int row = GroupingProxy::instance()->findPrevious( searchTerm, currentRow );
    if( row != -1 ) {
        //select this track
        debug() << "Got match at row: " << row;
        
        QModelIndex index = model()->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

        QModelIndex foundIndex = model()->index( row, 0, QModelIndex() );
        if ( foundIndex.isValid() )
            scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );
        
        emit( found() );
    } else {
        emit( notFound() );
    }
}

#include "PrettyListView.moc"