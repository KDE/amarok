/****************************************************************************************
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::PrettyListView"

#include "PrettyListView.h"

#include "amarokconfig.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "Debug.h"
#include "EngineController.h"
#include "PrettyItemDelegate.h"
#include "dialogs/TagDialog.h"
#include "GlobalCurrentTrackActions.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/capabilities/MultiSourceCapability.h"
#include "meta/Meta.h"
#include "PaletteHandler.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/view/PlaylistViewCommon.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"
#include "SourceSelectionPopup.h"

#include <KApplication>
#include <KMenu>

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
#include <QTimer>
#include "../../proxymodels/GroupingProxy.h"

Playlist::PrettyListView::PrettyListView( QWidget* parent )
        : QListView( parent )
        , EngineObserver( The::engineController() )
        , m_headerPressIndex( QModelIndex() )
        , m_mousePressInHeader( false )
        , m_skipAutoScroll( false )
        , m_pd( 0 )
        , m_topmostProxy( GroupingProxy::instance() )
{
    setModel( m_topmostProxy );
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

    m_proxyUpdateTimer = new QTimer( this );
    m_proxyUpdateTimer->setSingleShot( true );

    connect( m_proxyUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateProxyTimeout() ) );

    connect( m_topmostProxy, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( itemsAdded( const QModelIndex&, int, int ) ) );

    connect( m_topmostProxy, SIGNAL( layoutChanged() ), this, SLOT( reset() ) );
}

Playlist::PrettyListView::~PrettyListView() {}

void
Playlist::PrettyListView::engineNewTrackPlaying()
{
    if( AmarokConfig::autoScrollPlaylist() )
        scrollToActiveTrack();
}

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
Playlist::PrettyListView::playFirstSelected()
{
    QModelIndexList selected = selectedIndexes();
    if( !selected.isEmpty() )
        trackActivated( selected.first() );
}

void
Playlist::PrettyListView::removeSelection()
{
    QList<int> sr = selectedRows(); //these are in the source model
    if( !sr.isEmpty() )
    {
        // To select the right track after the removal, we need to get the first row in the
        // order defined by the topmost proxy.
        QList<int> selectedRowsInTopmostProxy;
        foreach( const QModelIndex &idx, selectedIndexes() )
        {
            int proxyRow = idx.row();
            selectedRowsInTopmostProxy.append( proxyRow );
        }

        // Now that we have the list of selected rows in the topmost proxy, we can perform the
        // removal.
        Controller::instance()->removeRows( sr );

        // Next, we look for the first row.
        int firstRow = selectedRowsInTopmostProxy.first();
        foreach( int i, selectedRowsInTopmostProxy )
        {
            if( i < firstRow )
                firstRow = i;
        }

        // Select the track immediately above the cleared are as this is the one that has internal focus.
        firstRow = qBound( 0, firstRow, m_topmostProxy->rowCount() -1 );
        selectionModel()->select( m_topmostProxy->index(  firstRow, 0, QModelIndex() ), QItemSelectionModel::Select );
    }
}

void
Playlist::PrettyListView::queueSelection()
{
    Actions::instance()->queue( selectedRows() );
}

void
Playlist::PrettyListView::dequeueSelection()
{
    Actions::instance()->dequeue( selectedRows() );
}

void Playlist::PrettyListView::selectSource()
{

    DEBUG_BLOCK

    QList<int> rows = selectedRows();

    //for now, bail out of more than 1 row...
    if ( rows.count() != 1 )
        return;

    //get the track...
    QModelIndex index = m_topmostProxy->index( rows.at( 0 ) );
    Meta::TrackPtr track = index.data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    //get multiSource capability:

    Meta::MultiSourceCapability *msc = track->create<Meta::MultiSourceCapability>();
    if ( msc )
    {
        debug() << "sources: " << msc->sources();
        SourceSelectionPopup * sourceSelector = new SourceSelectionPopup( this, msc );
        sourceSelector->show();
        //dialog deletes msc when done with it.
    }

}


void
Playlist::PrettyListView::scrollToActiveTrack()
{
    DEBUG_BLOCK
        debug() << "skipping scroll?" << m_skipAutoScroll;
    if( m_skipAutoScroll )
    {
        m_skipAutoScroll = false;
        return;
    }
    QModelIndex activeIndex = model()->index( m_topmostProxy->activeRow(), 0, QModelIndex() );
    if ( activeIndex.isValid() )
        scrollTo( activeIndex, QAbstractItemView::PositionAtCenter );
}

void
Playlist::PrettyListView::trackActivated( const QModelIndex& idx )
{
    DEBUG_BLOCK
    m_skipAutoScroll = true; // we don't want to do crazy view changes when selecting an item in the view
    Actions::instance()->play( idx );
}

void
Playlist::PrettyListView::showEvent( QShowEvent* event )
{
    QTimer::singleShot( 0, this, SLOT( fixInvisible() ) );

    QListView::showEvent( event );
}

// This method is a workaround for BUG 184714.
//
// It prevents the playlist from becoming invisible (clear) after changing the model, while Amarok is hidden in the tray.
// Without this workaround the playlist stays invisible when the application is restored from the tray.
// This is especially a problem with the Dynamic Playlist mode, which modifies the model without user interaction.
//
// The bug only seems to happen with Qt 4.5.x, so it might actually be a bug in Qt.
void
Playlist::PrettyListView::fixInvisible() //SLOT
{
    DEBUG_BLOCK

    // Part 1: Palette change
    newPalette( palette() );

    // Part 2: Change item selection
    const QItemSelection oldSelection( selectionModel()->selection() );
    selectionModel()->clear();
    selectionModel()->select( oldSelection, QItemSelectionModel::SelectCurrent );

    // NOTE: A simple update() call is not sufficient, but in fact the above two steps are required.
}

void
Playlist::PrettyListView::contextMenuEvent( QContextMenuEvent* event )
{
    DEBUG_BLOCK
    QModelIndex index = indexAt( event->pos() );

    if ( !index.isValid() )
        return;

    //Ctrl + Right Click is used for queuing
    if( event->modifiers() & Qt::ControlModifier )
        return;

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
Playlist::PrettyListView::stopAfterTrack()
{
    DEBUG_BLOCK
    const qint64 id = currentIndex().data( UniqueIdRole ).value<quint64>();
    if( Actions::instance()->willStopAfterTrack( id ) )
    {
        Actions::instance()->setStopAfterMode( StopNever );
        Actions::instance()->setTrackToBeLast( 0 );
    }
    else
    {
        Actions::instance()->setStopAfterMode( StopAfterQueue );
        Actions::instance()->setTrackToBeLast( id );
    }
}

void
Playlist::PrettyListView::dragMoveEvent( QDragMoveEvent* event )
{
    QModelIndex index = indexAt( event->pos() );
    if ( index.isValid() ) {
        m_dropIndicator = visualRect( index );
    } else {
        // draw it on the bottom of the last item
        index = model()->index( m_topmostProxy->rowCount() - 1, 0, QModelIndex() );
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
        m_headerPressIndex = QPersistentModelIndex( index );
        int rows = index.data( GroupedTracksRole ).toInt();
        QModelIndex bottomIndex = model()->index( index.row() + rows - 1, 0 );

        //offset by 1 as the actual header item is selected in QListView::mousePressEvent( event ); and is otherwise deselected again
        QItemSelection selItems( model()->index( index.row() + 1, 0 ), bottomIndex );
        QItemSelectionModel::SelectionFlags command = headerPressSelectionCommand( index, event );
        selectionModel()->select( selItems, command );
        // TODO: if you're doing shift-select on rows above the header, then the rows following the header will be lost from the selection
        selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
    }
    else
    {
        m_mousePressInHeader = false;
    }

    // This should always be forwarded, as it is used to determine the offset
    // relative to the mouse of the selection we are dragging!
    QListView::mousePressEvent( event );

    // This must go after the call to the super class as the current index is not yet selected otherwise
    // Queueing support for Ctrl Right click
    if( event->button() == Qt::RightButton && event->modifiers() & Qt::ControlModifier )
        queueSelection();
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
    return false;
}

void
Playlist::PrettyListView::paintEvent( QPaintEvent* event )
{
    if ( !m_dropIndicator.size().isEmpty() )
    {
        const QPoint offset( 6, 0 );
        const QPalette p = KApplication::palette();
        const QPen pen( p.color( QPalette::Highlight ), 6, Qt::SolidLine, Qt::RoundCap );
        QPainter painter( viewport() );
        painter.setPen( pen );
        painter.drawLine( m_dropIndicator.topLeft() + offset, m_dropIndicator.topRight() - offset );
    }

    QListView::paintEvent( event );
}

void
Playlist::PrettyListView::startDrag( Qt::DropActions supportedActions )
{
    DEBUG_BLOCK

    //Waah? when a parent item is dragged, startDrag is called a bunch of times
    static bool ongoingDrags = false;
    if( ongoingDrags )
        return;
    ongoingDrags = true;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {

        m_pd->setSvgRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
        qDebug() << "svgHandler SVG renderer is " << (QObject*)(The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ));
        qDebug() << "m_pd SVG renderer is " << (QObject*)(m_pd->svgRenderer());
        qDebug() << "does play exist in renderer? " << ( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" )->elementExists( "load" ) );
        QModelIndexList indices = selectedIndexes();

        QList<PopupDropperAction*> actions =  ViewCommon::actionsFor( this, &indices.first(), true );

        foreach( PopupDropperAction * action, actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ), true );

        m_pd->show();
    }

    QListView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( clear() ) );
        m_pd->hide();
    }
    ongoingDrags = false;
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
    return QItemSelectionModel::NoUpdate;
}

QList<int>
Playlist::PrettyListView::selectedRows() const
{
    QList<int> rows;
    foreach( const QModelIndex &idx, selectedIndexes() )
    {
        int sourceRow = FilterProxy::instance()->rowToSource( SortProxy::instance()->rowToSource( idx.row() ) );   //FIXME: this should use only m_topmostProxy
        rows.append( sourceRow );
    }
    return rows;
}

void Playlist::PrettyListView::newPalette( const QPalette & palette )
{
    Q_UNUSED( palette )
    The::paletteHandler()->updateItemView( this );
    reset();
}

void Playlist::PrettyListView::find( const QString &searchTerm, int fields, bool filter )
{
    bool updateProxy = false;
    if ( ( m_topmostProxy->currentSearchFields() != fields ) || ( m_topmostProxy->currentSearchTerm() != searchTerm ) )
        updateProxy = true;

    int row = m_topmostProxy->find( searchTerm, fields );
    if( row != -1 )
    {
        //select this track

        if ( !filter )
        {
            QModelIndex index = model()->index( row, 0 );
            QItemSelection selItems( index, index );
            selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

            QModelIndex foundIndex = model()->index( row, 0, QModelIndex() );
            setCurrentIndex( foundIndex );
            if ( foundIndex.isValid() )
                scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );
        }

        emit( found() );
    }
    else
        emit( notFound() );


    //instead of kicking the proxy right away, start a 500msec timeout.
    //this stops us from updating it for each letter of a long search term,
    //and since it does not affect any views, this is fine. Worst case is that
    //a navigator skips to a track form the old search if the track change happens
    //before this  timeout. Only start count if values have actually changed!
    if ( updateProxy )
        startProxyUpdateTimeout();
}

void Playlist::PrettyListView::findNext( const QString & searchTerm, int fields )
{
    DEBUG_BLOCK
    QList<int> selected = selectedRows();

    bool updateProxy = false;
    if ( ( m_topmostProxy->currentSearchFields() != fields ) || ( m_topmostProxy->currentSearchTerm() != searchTerm ) )
        updateProxy = true;

    int currentRow = -1;
    if( selected.size() > 0 )
        currentRow = selected.last();

    int row = m_topmostProxy->findNext( searchTerm, currentRow, fields );
    if( row != -1 )
    {
        //select this track

        QModelIndex index = m_topmostProxy->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

        QModelIndex foundIndex = m_topmostProxy->index( row, 0, QModelIndex() );
        setCurrentIndex( foundIndex );
        if ( foundIndex.isValid() )
            scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );

        emit( found() );
    }
    else
        emit( notFound() );

    if ( updateProxy )
        m_topmostProxy->filterUpdated();
}

void Playlist::PrettyListView::findPrevious( const QString & searchTerm, int fields )
{
    DEBUG_BLOCK
    QList<int> selected = selectedRows();

    bool updateProxy = false;
    if ( ( m_topmostProxy->currentSearchFields() != fields ) || ( m_topmostProxy->currentSearchTerm() != searchTerm ) )
        updateProxy = true;

    int currentRow = m_topmostProxy->totalLength();
    if( selected.size() > 0 )
        currentRow = selected.first();

    int row = m_topmostProxy->findPrevious( searchTerm, currentRow, fields );
    if( row != -1 )
    {
        //select this track

        QModelIndex index = m_topmostProxy->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

        QModelIndex foundIndex = m_topmostProxy->index( row, 0, QModelIndex() );
        setCurrentIndex( foundIndex );
        if ( foundIndex.isValid() )
            scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );

        emit( found() );
    }
    else
        emit( notFound() );

    if ( updateProxy )
        m_topmostProxy->filterUpdated();
}

void Playlist::PrettyListView::clearSearchTerm()
{
    DEBUG_BLOCK

    //We really do not want to reset the view to the top when the search/filter is cleared, so
    //we store the first shown row and scroll to that once the term is removed.
    QModelIndex index = indexAt( QPoint( 0, 0 ) );

     //ah... but we want the source row and not the one reported by the filter model(s)
    int row = FilterProxy::instance()->rowToSource( SortProxy::instance()->rowToSource( index.row() ) );    //FIXME

    debug() << "first row in filtered list: " << index.row();
    debug() << "source row: " << row;

    m_topmostProxy->filterUpdated();
    m_topmostProxy->clearSearchTerm();

    //now scroll to the selected row again

    QModelIndex sourceIndex = m_topmostProxy->index( row, 0, QModelIndex() );
    if ( sourceIndex.isValid() )
        scrollTo( sourceIndex, QAbstractItemView::PositionAtTop );
}

void Playlist::PrettyListView::startProxyUpdateTimeout()
{
    DEBUG_BLOCK
    if ( m_proxyUpdateTimer->isActive() )
        m_proxyUpdateTimer->stop();

    m_proxyUpdateTimer->setInterval( 500 );
    m_proxyUpdateTimer->start();
}

void Playlist::PrettyListView::updateProxyTimeout()
{
    DEBUG_BLOCK
    m_topmostProxy->filterUpdated();
}

void Playlist::PrettyListView::showOnlyMatches( bool onlyMatches )
{
    m_topmostProxy->showOnlyMatches( onlyMatches );
}

void Playlist::PrettyListView::itemsAdded( const QModelIndex& parent, int firstRow, int lastRow )
{
    DEBUG_BLOCK
    Q_UNUSED( parent )
    Q_UNUSED( lastRow )

    QModelIndex index = m_topmostProxy->index( firstRow, 0);
    if( !index.isValid() )
        return;

    debug() << "index has row: " << index.row();
    scrollTo( index, QAbstractItemView::PositionAtCenter );

}



#include "PrettyListView.moc"


