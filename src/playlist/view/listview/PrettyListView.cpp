/****************************************************************************************
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 John Atkinson <john@fauxnetic.co.uk>                              *
 * Copyright (c) 2009-2010 Oleksandr Khayrullin <saniokh@gmail.com>                     *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::PrettyListView"

#include "PrettyListView.h"

#include "amarokconfig.h"
#include "AmarokMimeData.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "dialogs/TagDialog.h"
#include "GlobalCurrentTrackActions.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/capabilities/MultiSourceCapability.h"
#include "core/meta/Meta.h"
#include "PaletteHandler.h"
#include "playlist/layouts/LayoutManager.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistController.h"
#include "playlist/view/PlaylistViewCommon.h"
#include "playlist/PlaylistDefines.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"
#include "SourceSelectionPopup.h"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDropEvent>
#include <QItemSelection>
#include <QKeyEvent>
#include <QListView>
#include <QMenu>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPersistentModelIndex>
#include <QScrollBar>
#include <QSvgRenderer>
#include <QTimer>
#include <QUrl>

#include <KLocalizedString>


Playlist::PrettyListView::PrettyListView( QWidget* parent )
        : QListView( parent )
        , ViewCommon()
        , m_headerPressIndex( QModelIndex() )
        , m_mousePressInHeader( false )
        , m_skipAutoScroll( false )
        , m_firstScrollToActiveTrack( true )
        , m_rowsInsertedScrollItem( 0 )
        , m_showOnlyMatches( false )
        , m_pd( nullptr )
{
    // QAbstractItemView basics
    setModel( The::playlist()->qaim() );

    m_prettyDelegate = new PrettyItemDelegate( this );
    connect( m_prettyDelegate, &PrettyItemDelegate::redrawRequested, this, &Playlist::PrettyListView::redrawActive );
    setItemDelegate( m_prettyDelegate );

    setSelectionMode( ExtendedSelection );
    setDragDropMode( DragDrop );
    setDropIndicatorShown( false ); // we draw our own drop indicator
    setEditTriggers ( SelectedClicked | EditKeyPressed );
    setAutoScroll( true );

    setVerticalScrollMode( ScrollPerPixel );

    setMouseTracking( true );

    // Rendering adjustments
    setFrameShape( QFrame::NoFrame );
    setAlternatingRowColors( true) ;
    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &PrettyListView::newPalette );

    setAutoFillBackground( false );


    // Signal connections
    connect( this, &Playlist::PrettyListView::doubleClicked,
             this, &Playlist::PrettyListView::trackActivated );
    connect( selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &Playlist::PrettyListView::slotSelectionChanged );

    connect( LayoutManager::instance(), &LayoutManager::activeLayoutChanged, this, &PrettyListView::playlistLayoutChanged );

    if (auto m = static_cast<Playlist::Model*>(model()))
    {
        connect( m, &Playlist::Model::activeTrackChanged, this, &Playlist::PrettyListView::slotPlaylistActiveTrackChanged );
        connect( m, &Playlist::Model::queueChanged, viewport(), QOverload<>::of(&QWidget::update) );
    }
    else
        warning() << "Model is not a Playlist::Model";

    //   Warning, this one doesn't connect to the normal 'model()' (i.e. '->top()'), but to '->bottom()'.
    connect( Playlist::ModelStack::instance()->bottom(), &Playlist::Model::rowsInserted, this, &Playlist::PrettyListView::bottomModelRowsInserted );

    // Timers
    m_proxyUpdateTimer = new QTimer( this );
    m_proxyUpdateTimer->setSingleShot( true );
    connect( m_proxyUpdateTimer, &QTimer::timeout, this, &Playlist::PrettyListView::updateProxyTimeout );

    m_animationTimer = new QTimer(this);
    connect( m_animationTimer, &QTimer::timeout, this, &Playlist::PrettyListView::redrawActive );
    m_animationTimer->setInterval( 250 );

    playlistLayoutChanged();

    // We do the following call here to be formally correct, but note:
    //   - It happens to be redundant, because 'playlistLayoutChanged()' already schedules
    //     another one, via a QTimer( 0 ).
    //   - Both that one and this one don't work right (they scroll like 'PositionAtTop',
    //     not 'PositionAtCenter'). This is probably because MainWindow changes its
    //     geometry in a QTimer( 0 )? As a fix, MainWindow does a 'slotShowActiveTrack()'
    //     at the end of its QTimer slot, which will finally scroll to the right spot.
    slotPlaylistActiveTrackChanged();
}

Playlist::PrettyListView::~PrettyListView()
{}

int
Playlist::PrettyListView::verticalOffset() const
{
    int ret = QListView::verticalOffset();
    if ( verticalScrollBar() && verticalScrollBar()->maximum() )
        ret += verticalScrollBar()->value() * 10 / verticalScrollBar()->maximum();
    return ret;
}

void
Playlist::PrettyListView::editTrackInformation()
{
    Meta::TrackList tl;
    for( const QModelIndex &index : selectedIndexes() )
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
    QList<int> sr = selectedRows();
    if( !sr.isEmpty() )
    {
        // Now that we have the list of selected rows in the topmost proxy, we can perform the
        // removal.
        The::playlistController()->removeRows( sr );

        // Next, we look for the first row.
        int firstRow = sr.first();
        for( int i : sr )
        {
            if( i < firstRow )
                firstRow = i;
        }

        //Select the track occupied by the first deleted track. Also move the current item to here as
        //button presses up or down wil otherwise not behave as expected.
        firstRow = qBound( 0, firstRow, model()->rowCount() - 1 );
        QModelIndex newSelectionIndex = model()->index( firstRow, 0 );
        setCurrentIndex( newSelectionIndex );
        selectionModel()->select( newSelectionIndex, QItemSelectionModel::Select );
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

void
Playlist::PrettyListView::switchQueueState() // slot
{
    DEBUG_BLOCK
    const bool isQueued = currentIndex().data( Playlist::QueuePositionRole ).toInt() != 0;
    isQueued ? dequeueSelection() : queueSelection();
}

void Playlist::PrettyListView::selectSource()
{
    DEBUG_BLOCK

    QList<int> rows = selectedRows();

    //for now, bail out of more than 1 row...
    if ( rows.count() != 1 )
        return;

    //get the track...
    QModelIndex index = model()->index( rows.at( 0 ), 0 );
    Meta::TrackPtr track = index.data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    //get multiSource capability:

    Capabilities::MultiSourceCapability *msc = track->create<Capabilities::MultiSourceCapability>();
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

    if( m_skipAutoScroll )
    {
        m_skipAutoScroll = false;
        return;
    }

    QModelIndex activeIndex = model()->index( The::playlist()->activeRow(), 0, QModelIndex() );
    if ( activeIndex.isValid() )
    {
        scrollTo( activeIndex, QAbstractItemView::PositionAtCenter );
        m_firstScrollToActiveTrack = false;
        m_rowsInsertedScrollItem = 0;    // This "new active track" scroll supersedes a pending "rows inserted" scroll.
    }
}

void
Playlist::PrettyListView::downOneTrack()
{
    DEBUG_BLOCK

    moveTrackSelection( 1 );
}

void
Playlist::PrettyListView::upOneTrack()
{
    DEBUG_BLOCK

    moveTrackSelection( -1 );
}

void
Playlist::PrettyListView::moveTrackSelection( int offset )
{
    if ( offset == 0 )
        return;

    int finalRow = model()->rowCount() - 1;
    int target = 0;

    if ( offset < 0 )
        target = finalRow;

    QList<int> rows = selectedRows();
    if ( rows.count() > 0 )
        target = rows.at( 0 ) + offset;

    target = qBound(0, target, finalRow);
    QModelIndex index = model()->index( target, 0 );
    setCurrentIndex( index );
}

void
Playlist::PrettyListView::slotPlaylistActiveTrackChanged()
{
    DEBUG_BLOCK

    // A playlist 'activeTrackChanged' signal happens:
    //   - During startup, on "saved playlist" load. (Might happen before this view exists)
    //   - When Amarok starts playing a new item in the playlist.
    //     In that case, don't auto-scroll if the user doesn't like us to.

    if( AmarokConfig::autoScrollPlaylist() || m_firstScrollToActiveTrack )
        scrollToActiveTrack();
}

void
Playlist::PrettyListView::slotSelectionChanged()
{
    m_lastTimeSelectionChanged = QDateTime::currentDateTime();
}

void
Playlist::PrettyListView::trackActivated( const QModelIndex& idx )
{
    DEBUG_BLOCK
    m_skipAutoScroll = true; // we don't want to do crazy view changes when selecting an item in the view
    Actions::instance()->play( idx );

    //make sure that the track we just activated is also set as the current index or
    //the selected index will get moved to the first row, making keyboard navigation difficult (BUG 225791)
    selectionModel_setCurrentIndex( idx, QItemSelectionModel::ClearAndSelect );

    setFocus();
}


// The following 2 functions are a workaround for crash BUG 222961 and BUG 229240:
//   There appears to be a bad interaction between Qt 'setCurrentIndex()' and
//   Qt 'selectedIndexes()' / 'selectionModel()->select()' / 'scrollTo()'.
//
//   'setCurrentIndex()' appears to do something bad with its QModelIndex parameter,
//   leading to a crash deep within Qt.
//
//   It might be our fault, but we suspect a bug in Qt.  (Qt 4.6 at least)
//
//   The problem goes away if we use a fresh QModelIndex, which we also don't re-use
//   afterwards.
void
Playlist::PrettyListView::setCurrentIndex( const QModelIndex &index )
{
    QModelIndex indexCopy = model()->index( index.row(), index.column() );
    QListView::setCurrentIndex( indexCopy );
}

void
Playlist::PrettyListView::selectionModel_setCurrentIndex( const QModelIndex &index, QItemSelectionModel::SelectionFlags command )
{
    QModelIndex indexCopy = model()->index( index.row(), index.column() );
    selectionModel()->setCurrentIndex( indexCopy, command );
}

void
Playlist::PrettyListView::showEvent( QShowEvent* event )
{
    QTimer::singleShot( 0, this, &Playlist::PrettyListView::fixInvisible );

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
    // DEBUG_BLOCK

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

    trackMenu( this, &index, event->globalPos() );
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
    const quint64 id = currentIndex().data( UniqueIdRole ).value<quint64>();
    if( Actions::instance()->willStopAfterTrack( id ) )
        Actions::instance()->stopAfterPlayingTrack( 0 ); // disable stopping
    else
        Actions::instance()->stopAfterPlayingTrack( id );
}

void
Playlist::PrettyListView::findInSource()
{
    DEBUG_BLOCK

    Meta::TrackPtr track = currentIndex().data( TrackRole ).value<Meta::TrackPtr>();
    if ( track )
    {
        if( track->has<Capabilities::FindInSourceCapability>() )
        {
            Capabilities::FindInSourceCapability *fis = track->create<Capabilities::FindInSourceCapability>();
            if ( fis )
            {
                fis->findInSource();
            }
            delete fis;
        }
    }
}

void
Playlist::PrettyListView::dragEnterEvent( QDragEnterEvent *event )
{
    const QMimeData *mime = event->mimeData();
    if( mime->hasUrls() ||
        mime->hasFormat( AmarokMimeData::TRACK_MIME ) ||
        mime->hasFormat( AmarokMimeData::PLAYLIST_MIME ) ||
        mime->hasFormat( AmarokMimeData::PODCASTEPISODE_MIME ) ||
        mime->hasFormat( AmarokMimeData::PODCASTCHANNEL_MIME ) )
    {
        event->acceptProposedAction();
    }
}

void
Playlist::PrettyListView::dragMoveEvent( QDragMoveEvent* event )
{
    QModelIndex index = indexAt( event->pos() );
    if ( index.isValid() )
    {
        m_dropIndicator = visualRect( index );
    }
    else
    {
        // draw it on the bottom of the last item
        index = model()->index( model()->rowCount() - 1, 0, QModelIndex() );
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
    if ( qobject_cast<PrettyListView*>( event->source() ) == this )
    {
        QAbstractItemModel* plModel = model();
        int targetRow = indexAt( event->pos() ).row();
        targetRow = ( targetRow < 0 ) ? plModel->rowCount() : targetRow; // target of < 0 means we dropped on the end of the playlist
        QList<int> sr = selectedRows();
        int realtarget = The::playlistController()->moveRows( sr, targetRow );
        QItemSelection selItems;
        for( int row : sr )
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
Playlist::PrettyListView::keyPressEvent( QKeyEvent *event )
{
    if( event->matches( QKeySequence::Delete ) )
    {
        removeSelection();
        event->accept();
    }
    else if( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
    {
        trackActivated( currentIndex() );
        event->accept();
    }
    else if( event->matches( QKeySequence::SelectAll ) )
    {
        QModelIndex topIndex = model()->index( 0, 0 );
        QModelIndex bottomIndex = model()->index( model()->rowCount() - 1, 0 );
        QItemSelection selItems( topIndex, bottomIndex );
        selectionModel()->select( selItems, QItemSelectionModel::ClearAndSelect );
        event->accept();
    }
    else
        QListView::keyPressEvent( event );
}

void
Playlist::PrettyListView::mousePressEvent( QMouseEvent* event )
{
    //get the item that was clicked
    QModelIndex index = indexAt( event->pos() );

    //first of all, if a left click, check if the delegate wants to do something about this click
    if( event->button() == Qt::LeftButton )
    {
        //we need to translate the position of the click into something relative to the item that was clicked.
        QRect itemRect = visualRect( index );
        QPoint relPos =  event->pos() - itemRect.topLeft();

        if( m_prettyDelegate->clicked( relPos, itemRect, index ) )
        {
            event->accept();
            return;  //click already handled...
        }
    }

    if ( mouseEventInHeader( event ) && ( event->button() == Qt::LeftButton ) )
    {
        m_mousePressInHeader = true;
        m_headerPressIndex = QPersistentModelIndex( index );
        int rows = index.data( GroupedTracksRole ).toInt();
        QModelIndex bottomIndex = model()->index( index.row() + rows - 1, 0 );

        //offset by 1 as the actual header item is selected in QListView::mousePressEvent( event ); and is otherwise deselected again
        QItemSelection selItems( model()->index( index.row() + 1, 0 ), bottomIndex );
        QItemSelectionModel::SelectionFlags command = headerPressSelectionCommand( index, event );
        selectionModel()->select( selItems, command );
        // TODO: if you're doing shift-select on rows above the header, then the rows following the header will be lost from the selection
        selectionModel_setCurrentIndex( index, QItemSelectionModel::NoUpdate );
    }
    else
    {
        m_mousePressInHeader = false;
    }

    if ( event->button() == Qt::MiddleButton )
    {
        QUrl url( QApplication::clipboard()->text() );
        if ( url.isValid() )
        {
            QList<QUrl> urls = QList<QUrl>() << url;
            if( index.isValid() )
                The::playlistController()->insertUrls( index.row() + 1, urls );
            else
                The::playlistController()->insertOptioned( urls, Playlist::OnAppendToPlaylistAction );
        }
    }

    // This should always be forwarded, as it is used to determine the offset
    // relative to the mouse of the selection we are dragging!
    QListView::mousePressEvent( event );

    // This must go after the call to the super class as the current index is not yet selected otherwise
    // Queueing support for Ctrl Right click
    if( event->button() == Qt::RightButton && event->modifiers() & Qt::ControlModifier )
    {
        QList<int> list;
        if (selectedRows().contains( index.row()) )
        {
            // select all selected rows if mouse is over selection area
            list = selectedRows();
        }
        else
        {
            // select only current mouse-over-index if mouse is out of selection area
            list.append( index.row() );
        }

        if( index.data( Playlist::QueuePositionRole ).toInt() != 0 )
            Actions::instance()->dequeue( list );
        else
            Actions::instance()->queue( list );
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
    if ( index.data( GroupRole ).toInt() == Grouping::Head )
    {
        QPoint mousePressPos = event->pos();
        mousePressPos.rx() += horizontalOffset();
        mousePressPos.ry() += verticalOffset();
        return m_prettyDelegate->insideItemHeader( mousePressPos, rectForIndex( index ) );
    }
    return false;
}

void
Playlist::PrettyListView::paintEvent( QPaintEvent *event )
{
    if( m_dropIndicator.isValid() ||
        model()->rowCount( rootIndex() ) == 0 )
    {
        QPainter painter( viewport() );

        if( m_dropIndicator.isValid() )
        {
            const QPoint offset( 6, 0 );
            QColor c = QApplication::palette().color( QPalette::Highlight );
            painter.setPen( QPen( c, 6, Qt::SolidLine, Qt::RoundCap ) );
            painter.drawLine( m_dropIndicator.topLeft() + offset,
                              m_dropIndicator.topRight() - offset );
        }

        if( model()->rowCount( rootIndex() ) == 0 )
        {
            // here we assume that an empty list is caused by the filter if it's active
            QString emptyText;
            if( m_showOnlyMatches && Playlist::ModelStack::instance()->bottom()->rowCount() > 0 )
                emptyText = i18n( "Tracks have been hidden due to the active search." );
            else
                emptyText = i18nc( "Placeholder message in empty playlist view", "Add some tracks here by dragging them from all around." );

            QColor c = QApplication::palette().color( foregroundRole() );
            c.setAlpha( c.alpha() / 2 );
            painter.setPen( c );
            painter.drawText( rect(),
                              Qt::AlignCenter | Qt::TextWordWrap,
                              emptyText );
        }
    }

    QListView::paintEvent( event );
}

void
Playlist::PrettyListView::startDrag( Qt::DropActions supportedActions )
{
    DEBUG_BLOCK

    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return; // no items selected in the view, abort. See bug 226167

    //Waah? when a parent item is dragged, startDrag is called a bunch of times
    static bool ongoingDrags = false;
    if( ongoingDrags )
        return;
    ongoingDrags = true;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {
        m_pd->setSvgRenderer( The::svgHandler()->getRenderer( QStringLiteral("amarok/images/pud_items.svg") ) );
        qDebug() << "svgHandler SVG renderer is " << (QObject*)(The::svgHandler()->getRenderer( QStringLiteral("amarok/images/pud_items.svg") ));
        qDebug() << "m_pd SVG renderer is " << (QObject*)(m_pd->svgRenderer());
        qDebug() << "does play exist in renderer? " << ( The::svgHandler()->getRenderer( QStringLiteral("amarok/images/pud_items.svg") )->elementExists( QStringLiteral("load") ) );

        QList<QAction*> actions =  actionsFor( this, &indices.first() );
        for( QAction * action : actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ), true );

        m_pd->show();
    }

    QListView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, &PopupDropper::fadeHideFinished, m_pd, &PopupDropper::clear );
        m_pd->hide();
    }
    ongoingDrags = false;
}

bool
Playlist::PrettyListView::edit( const QModelIndex &index, EditTrigger trigger, QEvent *event )
{
    // we want to prevent a click to change the selection and open the editor (BR 220818)
    if( m_lastTimeSelectionChanged.msecsTo( QDateTime::currentDateTime() ) < qApp->doubleClickInterval() + 50 )
        return false;
    return QListView::edit( index, trigger, event );
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
    for( const QModelIndex &idx : selectedIndexes() )
        rows.append( idx.row() );
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
    if ( ( The::playlist()->currentSearchFields() != fields ) || ( The::playlist()->currentSearchTerm() != searchTerm ) )
        updateProxy = true;

    m_searchTerm = searchTerm;
    m_fields = fields;
    m_filter = filter;

    int row = The::playlist()->find( m_searchTerm, m_fields );
    if( row != -1 )
    {
        //select this track
        QModelIndex index = model()->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );
    }

    //instead of kicking the proxy right away, start a small timeout.
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
    if ( ( The::playlist()->currentSearchFields() != fields ) || ( The::playlist()->currentSearchTerm() != searchTerm ) )
        updateProxy = true;

    int currentRow = -1;
    if( selected.size() > 0 )
        currentRow = selected.last();

    int row = The::playlist()->findNext( searchTerm, currentRow, fields );
    if( row != -1 )
    {
        //select this track

        QModelIndex index = model()->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

        QModelIndex foundIndex = model()->index( row, 0, QModelIndex() );
        setCurrentIndex( foundIndex );
        if ( foundIndex.isValid() )
            scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );

        Q_EMIT( found() );
    }
    else
        Q_EMIT( notFound() );

    if ( updateProxy )
        The::playlist()->filterUpdated();
}

void Playlist::PrettyListView::findPrevious( const QString & searchTerm, int fields )
{
    DEBUG_BLOCK
    QList<int> selected = selectedRows();

    bool updateProxy = false;
    if ( ( The::playlist()->currentSearchFields() != fields ) || ( The::playlist()->currentSearchTerm() != searchTerm ) )
        updateProxy = true;

    int currentRow = model()->rowCount();
    if( selected.size() > 0 )
        currentRow = selected.first();

    int row = The::playlist()->findPrevious( searchTerm, currentRow, fields );
    if( row != -1 )
    {
        //select this track

        QModelIndex index = model()->index( row, 0 );
        QItemSelection selItems( index, index );
        selectionModel()->select( selItems, QItemSelectionModel::SelectCurrent );

        QModelIndex foundIndex = model()->index( row, 0, QModelIndex() );
        setCurrentIndex( foundIndex );
        if ( foundIndex.isValid() )
            scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );

        Q_EMIT( found() );
    }
    else
        Q_EMIT( notFound() );

    if ( updateProxy )
        The::playlist()->filterUpdated();
}

void Playlist::PrettyListView::clearSearchTerm()
{
    DEBUG_BLOCK

    // Choose a focus item, to scroll to later.
    QModelIndex focusIndex;
    QModelIndexList selected = selectedIndexes();
    if( !selected.isEmpty() )
        focusIndex = selected.first();
    else
        focusIndex = indexAt( QPoint( 0, 0 ) );

    // Remember the focus item id, because the row numbers change when we reset the filter.
    quint64 focusItemId = The::playlist()->idAt( focusIndex.row() );

    The::playlist()->clearSearchTerm();
    The::playlist()->filterUpdated();

    // Now scroll to the focus item.
    QModelIndex newIndex = model()->index( The::playlist()->rowForId( focusItemId ), 0, QModelIndex() );
    if ( newIndex.isValid() )
        scrollTo( newIndex, QAbstractItemView::PositionAtCenter );
}

void Playlist::PrettyListView::startProxyUpdateTimeout()
{
    DEBUG_BLOCK
    if ( m_proxyUpdateTimer->isActive() )
        m_proxyUpdateTimer->stop();

    m_proxyUpdateTimer->setInterval( 200 );
    m_proxyUpdateTimer->start();
}

void Playlist::PrettyListView::updateProxyTimeout()
{
    DEBUG_BLOCK
    The::playlist()->filterUpdated();

    int row = The::playlist()->find( m_searchTerm, m_fields );
    if( row != -1 )
    {
        QModelIndex foundIndex = model()->index( row, 0, QModelIndex() );
        setCurrentIndex( foundIndex );

        if ( !m_filter )
        {
            if ( foundIndex.isValid() )
                scrollTo( foundIndex, QAbstractItemView::PositionAtCenter );
        }

        Q_EMIT( found() );
    }
    else
        Q_EMIT( notFound() );
}

void Playlist::PrettyListView::showOnlyMatches( bool onlyMatches )
{
    m_showOnlyMatches = onlyMatches;

    The::playlist()->showOnlyMatches( onlyMatches );
}

// Handle scrolling to newly inserted playlist items.
// Warning, this slot is connected to the 'rowsInserted' signal of the *bottom* model,
// not the normal top model.
// The reason: FilterProxy can Q_EMIT *A LOT* (thousands) of 'rowsInserted' signals when its
// search string changes. For that case we don't want to do any scrollTo() at all.
void
Playlist::PrettyListView::bottomModelRowsInserted( const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent )
    Q_UNUSED( end )

    // skip scrolling if tracks were added while playlist is in dynamicMode
    if( m_rowsInsertedScrollItem == 0 && !AmarokConfig::dynamicMode() )
    {
        m_rowsInsertedScrollItem = Playlist::ModelStack::instance()->bottom()->idAt( start );
        QTimer::singleShot( 0, this, &Playlist::PrettyListView::bottomModelRowsInsertedScroll );
    }
}

void Playlist::PrettyListView::bottomModelRowsInsertedScroll()
{
    DEBUG_BLOCK

    if( m_rowsInsertedScrollItem )
    {   // Note: we don't bother handling the case "first inserted item in bottom model
        // does not have a row in the top 'model()' due to FilterProxy" nicely.
        int firstRowInserted = The::playlist()->rowForId( m_rowsInsertedScrollItem );    // In the *top* model.
        QModelIndex index = model()->index( firstRowInserted, 0 );

        if( index.isValid() )
            scrollTo( index, QAbstractItemView::PositionAtCenter );

        m_rowsInsertedScrollItem = 0;
    }
}

void Playlist::PrettyListView::redrawActive()
{
    int activeRow = The::playlist()->activeRow();
    QModelIndex index = model()->index( activeRow, 0, QModelIndex() );
    update( index );
}

void Playlist::PrettyListView::playlistLayoutChanged()
{
    if ( LayoutManager::instance()->activeLayout().inlineControls() )
        m_animationTimer->start();
    else
        m_animationTimer->stop();

    // -- update the tooltip columns in the playlist model
    bool tooltipColumns[Playlist::NUM_COLUMNS];
    for( int i=0; i<Playlist::NUM_COLUMNS; ++i )
        tooltipColumns[i] = true;

    // bool excludeCover = false;

    for( int part = 0; part < PlaylistLayout::NumParts; part++ )
    {
        // bool single = ( part == PlaylistLayout::Single );
        Playlist::PlaylistLayout layout = Playlist::LayoutManager::instance()->activeLayout();
        Playlist::LayoutItemConfig item = layout.layoutForPart( (PlaylistLayout::Part)part );

        for (int activeRow = 0; activeRow < item.rows(); activeRow++)
        {
            for (int activeElement = 0; activeElement < item.row(activeRow).count();activeElement++)
            {
                Playlist::Column column = (Playlist::Column)item.row(activeRow).element(activeElement).value();
                tooltipColumns[column] = false;
            }
        }
        // excludeCover |= item.showCover();
    }
    Playlist::Model::setTooltipColumns( tooltipColumns );
    Playlist::Model::enableToolTip( Playlist::LayoutManager::instance()->activeLayout().tooltips() );

    update();

    // Schedule a re-scroll to the active playlist row. Assumption: Qt will run this *after* the repaint.
    QTimer::singleShot( 0, this, &Playlist::PrettyListView::slotPlaylistActiveTrackChanged );
}

