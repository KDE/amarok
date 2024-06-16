/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "BookmarkTreeView.h"

#include "BookmarkModel.h"
#include "dialogs/TagDialog.h"
#include "PaletteHandler.h"
#include "AmarokUrl.h"
#include "AmarokUrlHandler.h"
#include "BookmarkGroup.h"
#include "playlist/PlaylistController.h"
#include "SvgHandler.h"
#include "core-impl/meta/timecode/TimecodeMeta.h"

#include <QAction>
#include <QMenu>
#include <KLocalizedString>

#include <QHeaderView>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QModelIndex>
#include <QPoint>
#include <QToolTip>

#include <typeinfo>

BookmarkTreeView::BookmarkTreeView( QWidget *parent )
    : QTreeView( parent )
    , m_loadAction( nullptr )
    , m_deleteAction( nullptr )
    , m_createTimecodeTrackAction( nullptr )
    , m_addGroupAction( nullptr )
{

    setEditTriggers( QAbstractItemView::SelectedClicked );
    setSelectionMode( QAbstractItemView::ExtendedSelection );

    setDragEnabled( true );
    setAcceptDrops( true );
    setAlternatingRowColors( true );
    setDropIndicatorShown( true );

    connect( header(), &QHeaderView::sectionCountChanged,
             this, &BookmarkTreeView::slotSectionCountChanged );
}


BookmarkTreeView::~BookmarkTreeView()
{
}

void BookmarkTreeView::mouseDoubleClickEvent( QMouseEvent * event )
{
    QModelIndex index = m_proxyModel->mapToSource( indexAt( event->pos() ) );

    if( index.isValid() )
    {
        BookmarkViewItemPtr item = BookmarkModel::instance()->data( index, 0xf00d ).value<BookmarkViewItemPtr>();

        if ( auto bookmark = AmarokUrlPtr::dynamicCast( item ) )
            bookmark->run();
    }
}


void
BookmarkTreeView::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_Delete:
            slotDelete();
            return;

        case Qt::Key_F2:
            slotRename();
            return;
    }
    QTreeView::keyPressEvent( event );
}

QList<QAction *>
BookmarkTreeView::createCommonActions( const QModelIndexList &indices )
{
    DEBUG_BLOCK

    //there are 4 columns, so for each selected row we get 4 indices...
    int selectedRowCount = indices.count() / 4;

    QList< QAction * > actions;
    if ( m_loadAction == nullptr )
    {
        m_loadAction = new QAction( QIcon::fromTheme( QStringLiteral("folder-open") ), i18nc( "Load the view represented by this bookmark", "&Load" ), this );
        connect( m_loadAction, &QAction::triggered, this, &BookmarkTreeView::slotLoad );
    }

    if ( m_deleteAction == nullptr )
    {
        m_deleteAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-remove-amarok") ), i18n( "&Delete" ), this );
        connect( m_deleteAction, &QAction::triggered, this, &BookmarkTreeView::slotDelete );
    }

    if ( m_createTimecodeTrackAction == nullptr )
    {
        debug() << "creating m_createTimecodeTrackAction";
        m_createTimecodeTrackAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-edit-amarok") ), i18n( "&Create timecode track" ), this );
        connect( m_createTimecodeTrackAction, &QAction::triggered, this, &BookmarkTreeView::slotCreateTimecodeTrack );
    }

    if ( selectedRowCount == 1 )
        actions << m_loadAction;

    if ( selectedRowCount > 0 )
        actions << m_deleteAction;

    if ( selectedRowCount == 2 ) {
        debug() << "adding m_createTimecodeTrackAction";
        actions << m_createTimecodeTrackAction;
    }

    return actions;
}

void BookmarkTreeView::slotLoad()
{
    DEBUG_BLOCK
    for( BookmarkViewItemPtr item : selectedItems() )
    {
        if( auto bookmark = AmarokUrlPtr::dynamicCast( item ) )
            bookmark->run();
    }
}

void BookmarkTreeView::slotDelete()
{
    DEBUG_BLOCK

    //TODO FIXME Confirmation of delete

    for( BookmarkViewItemPtr item : selectedItems() )
    {
        debug() << "deleting " << item->name();
        item->removeFromDb();
        item->parent()->deleteChild( item );
    }
    BookmarkModel::instance()->reloadFromDb();
    The::amarokUrlHandler()->updateTimecodes();
}

void BookmarkTreeView::slotRename()
{
    DEBUG_BLOCK
    if ( selectionModel()->hasSelection() )
        edit( selectionModel()->selectedIndexes().first() );
}

void BookmarkTreeView::contextMenuEvent( QContextMenuEvent * event )
{
    DEBUG_BLOCK

    const QModelIndexList indices = selectionModel()->selectedIndexes();

    QMenu* menu = new QMenu( this );
    const QList<QAction *> actions = createCommonActions( indices );

    for ( QAction * action : actions )
        menu->addAction( action );

    if( indices.isEmpty() && m_addGroupAction)
        menu->addAction( m_addGroupAction );

    if (!menu->isEmpty()) {
        menu->exec( event->globalPos() );
    }
    delete menu;
}

void BookmarkTreeView::resizeEvent( QResizeEvent *event )
{
    QHeaderView *headerView = header();

    const int oldWidth = event->oldSize().width();
    const int newWidth = event->size().width();

    if( oldWidth == newWidth || oldWidth < 0 || newWidth < 0 )
        return;

    disconnect( headerView, &QHeaderView::sectionResized,
                this, &BookmarkTreeView::slotSectionResized );

    QMap<BookmarkModel::Column, qreal>::const_iterator i = m_columnsSize.constBegin();
    while( i != m_columnsSize.constEnd() )
    {
        const BookmarkModel::Column col = i.key();
        if( col != BookmarkModel::Command && col != BookmarkModel::Description )
            headerView->resizeSection( col, static_cast<int>( i.value() * newWidth ) );
        ++i;
    }

    connect( headerView, &QHeaderView::sectionResized,
             this, &BookmarkTreeView::slotSectionResized );

    QWidget::resizeEvent( event );
}

bool BookmarkTreeView::viewportEvent( QEvent *event )
{
   if( event->type() == QEvent::ToolTip )
   {
       QHelpEvent *he  = static_cast<QHelpEvent*>( event );
       QModelIndex idx = indexAt( he->pos() );

       if( idx.isValid() )
       {
           QRect vr  = visualRect( idx );
           QSize shr = itemDelegate( idx )->sizeHint( viewOptions(), idx );

           if( shr.width() > vr.width() )
               QToolTip::showText( he->globalPos(), idx.data( Qt::DisplayRole ).toString() );
       }
       else
       {
           QToolTip::hideText();
           event->ignore();
       }
       return true;
   }
   return QTreeView::viewportEvent( event );
}

QSet<BookmarkViewItemPtr>
BookmarkTreeView::selectedItems() const
{
    DEBUG_BLOCK
    QSet<BookmarkViewItemPtr> selected;
    for( const QModelIndex &index : selectionModel()->selectedIndexes() )
    {
        QModelIndex sourceIndex = m_proxyModel->mapToSource( index );
        if( sourceIndex.isValid() && sourceIndex.internalPointer() && sourceIndex.column() == 0 )
        {
            debug() << "inserting item " << sourceIndex.data( Qt::DisplayRole ).toString();
            selected.insert( BookmarkModel::instance()->data( sourceIndex, 0xf00d ).value<BookmarkViewItemPtr>() );
        }
    } 
    return selected;
}

void BookmarkTreeView::setNewGroupAction( QAction * action )
{
    m_addGroupAction = action;
}

void BookmarkTreeView::selectionChanged( const QItemSelection & selected, const QItemSelection & deselected )
{
    DEBUG_BLOCK
    Q_UNUSED( deselected )
    QModelIndexList indexes = selected.indexes();
    debug() << indexes.size() << " items selected";
    for( const QModelIndex &index : indexes )
    {
        const QModelIndex sourceIndex = m_proxyModel->mapToSource( index );
        if( sourceIndex.column() == 0 )
        {
            BookmarkViewItemPtr item = BookmarkModel::instance()->data( sourceIndex, 0xf00d ).value<BookmarkViewItemPtr>();

            if ( auto bookmark = AmarokUrlPtr::dynamicCast( item ) )
            {
                debug() << "a url was selected...";
                Q_EMIT( bookmarkSelected( *bookmark ) );
            }
        }
    }
    
}

QMenu* BookmarkTreeView::contextMenu( const QPoint& point )
{
    DEBUG_BLOCK
    QMenu* menu = new QMenu( nullptr );

    debug() << "getting menu for point:" << point;
    QModelIndex index = m_proxyModel->mapToSource( indexAt( point ) );
    if( index.isValid() )
    {

        debug() << "got valid index";
        
        QModelIndexList indices = selectionModel()->selectedIndexes();

        QList<QAction *> actions = createCommonActions( indices );

        for( QAction * action : actions )
            menu->addAction( action );

        if( indices.isEmpty() )
            menu->addAction( m_addGroupAction );

    }
    
    return menu;
}

void BookmarkTreeView::slotCreateTimecodeTrack() const
{

    //TODO: Factor into separate class
    QList<BookmarkViewItemPtr> list = selectedItems().values();
    if ( list.count() != 2 )
        return;

    const AmarokUrl * url1 = dynamic_cast<const AmarokUrl *>( list.at( 0 ).data() );

    if ( url1 == nullptr )
        return;
    if ( url1->command() != QLatin1String("play") )
        return;

    const AmarokUrl * url2 = dynamic_cast<const AmarokUrl *>( list.at( 1 ).data() );

    if ( url2 == nullptr )
        return;
    if ( url2->command() != QLatin1String("play") )
        return;

    if ( url1->path() != url2->path() )
        return;

    //ok, so we actually have to timecodes from the same base url, not get the
    //minimum and maximum time:
    qreal pos1 = 0;
    qreal pos2 = 0;

    if ( url1->args().keys().contains( QStringLiteral("pos") ) )
    {
        pos1 = url1->args().value( QStringLiteral("pos") ).toDouble();
    }

    if ( url2->args().keys().contains( QStringLiteral("pos") ) )
    {
        pos2 = url2->args().value( QStringLiteral("pos") ).toDouble();
    }

    if ( pos1 == pos2 )
        return;

    qint64 start = qMin( pos1, pos2 ) * 1000;
    qint64 end = qMax( pos1, pos2 ) * 1000;

    //Now we really should pop up a menu to get the user to enter some info about this
    //new track, but for now, just fake it as this is just for testing anyway

    QUrl url = QUrl::fromEncoded ( QByteArray::fromBase64 ( url1->path().toUtf8() ) );
    Meta::TimecodeTrackPtr track = Meta::TimecodeTrackPtr( new Meta::TimecodeTrack( i18n( "New Timecode Track" ), url, start, end ) );
    Meta::TimecodeAlbumPtr album = Meta::TimecodeAlbumPtr( new Meta::TimecodeAlbum( i18n( "Unknown" ) ) );
    Meta::TimecodeArtistPtr artist = Meta::TimecodeArtistPtr( new Meta::TimecodeArtist( i18n(  "Unknown" ) ) );
    Meta::TimecodeGenrePtr genre = Meta::TimecodeGenrePtr( new Meta::TimecodeGenre( i18n( "Unknown" ) ) );

    album->addTrack( track );
    artist->addTrack( track );
    genre->addTrack( track );

    track->setAlbum( album );
    track->setArtist( artist );
    track->setGenre( genre );

    album->setAlbumArtist( artist );

    //make the user give us some info about this item...

    Meta::TrackList tl;
    tl.append( Meta::TrackPtr::staticCast( track ) );
    TagDialog *dialog = new TagDialog( tl, nullptr );
    dialog->show();

    //now add it to the playlist
    The::playlistController()->insertOptioned( Meta::TrackPtr::staticCast( track ) );
}

void BookmarkTreeView::setProxy( QSortFilterProxyModel *proxy )
{
    m_proxyModel = proxy;
}

void BookmarkTreeView::slotEdit( const QModelIndex &index )
{

    //translate to proxy terms
    edit( m_proxyModel->mapFromSource( index ) );
}

void BookmarkTreeView::slotSectionResized( int logicalIndex, int oldSize, int newSize )
{
    Q_UNUSED( oldSize )
    BookmarkModel::Column col = BookmarkModel::Column( logicalIndex );
    m_columnsSize[ col ] = static_cast<qreal>( newSize ) / header()->length();
}

void BookmarkTreeView::slotSectionCountChanged( int oldCount, int newCount )
{
    Q_UNUSED( oldCount )

    const QHeaderView *headerView = header();
    for( int i = 0; i < newCount; ++i )
    {
        const int index   = headerView->logicalIndex( i );
        const int width   = columnWidth( index );
        const qreal ratio = static_cast<qreal>( width ) / headerView->length();

        const BookmarkModel::Column col = BookmarkModel::Column( index );

        if( col == BookmarkModel::Command )
            header()->setSectionResizeMode( index, QHeaderView::ResizeToContents );

        m_columnsSize[ col ] = ratio;
    }
}





