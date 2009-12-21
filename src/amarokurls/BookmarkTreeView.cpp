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
#include "statusbar/StatusBar.h"
#include "timecode/TimecodeMeta.h"

#include <KAction>
#include <KMenu>

#include <QFrame>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QModelIndex>
#include <QPoint>

#include <typeinfo>

BookmarkTreeView::BookmarkTreeView( QWidget *parent )
    : QTreeView( parent )
    , m_loadAction( 0 )
    , m_deleteAction( 0 )
    , m_createTimecodeTrackAction( 0 )
    , m_addGroupAction( 0 )
{

    setEditTriggers( QAbstractItemView::SelectedClicked );
    setSelectionMode( QAbstractItemView::ExtendedSelection );

    setDragEnabled( true );
    setAcceptDrops( true );
    setDropIndicatorShown( true );
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

        if ( typeid( *item ) == typeid( AmarokUrl ) ) {
            AmarokUrl * bookmark = static_cast< AmarokUrl* >( item.data() );
            bookmark->run();
        }
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

QList<KAction *>
BookmarkTreeView::createCommonActions( QModelIndexList indices )
{
    DEBUG_BLOCK

    //there are 4 colums, so for each selected row we get 4 indices...
    int selectedRowCount = indices.count() / 4;

    QList< KAction * > actions;
    
    if ( m_loadAction == 0 )
    {
        m_loadAction = new KAction( KIcon( "folder-open" ), i18nc( "Load the view represented by this bookmark", "&Load" ), this );
        connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotLoad() ) );
    }

    if ( m_deleteAction == 0 )
    {
        m_deleteAction = new KAction( KIcon( "media-track-remove-amarok" ), i18n( "&Delete" ), this );
        connect( m_deleteAction, SIGNAL( triggered() ), this, SLOT( slotDelete() ) );
    }

    if ( m_createTimecodeTrackAction == 0 )
    {
        debug() << "creating m_createTimecodeTrackAction";
        m_createTimecodeTrackAction = new KAction( KIcon( "media-track-edit-amarok" ), i18n( "&Create timecode track" ), this );
        connect( m_createTimecodeTrackAction, SIGNAL( triggered() ), this, SLOT( slotCreateTimecodeTrack() ) );
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
    foreach( BookmarkViewItemPtr item, selectedItems() )
    {
        if( typeid( * item ) == typeid( AmarokUrl ) )
        {
            AmarokUrlPtr bookmark = AmarokUrlPtr::staticCast( item );
            bookmark->run();
        }
    }
}

void BookmarkTreeView::slotDelete()
{
    DEBUG_BLOCK

    //TODO FIXME Confirmation of delete

    foreach( BookmarkViewItemPtr item, selectedItems() )
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

    QModelIndexList indices = selectionModel()->selectedIndexes();

    KMenu* menu = new KMenu( this );

    QList<KAction *> actions = createCommonActions( indices );

    foreach( KAction * action, actions )
        menu->addAction( action );

    if( indices.count() == 0 )
        menu->addAction( m_addGroupAction );

    menu->exec( event->globalPos() );
}

QSet<BookmarkViewItemPtr>
BookmarkTreeView::selectedItems() const
{
    DEBUG_BLOCK
    QSet<BookmarkViewItemPtr> selected;
    foreach( const QModelIndex &index, selectionModel()->selectedIndexes() )
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

void BookmarkTreeView::setNewGroupAction( KAction * action )
{
    m_addGroupAction = action;
}

void BookmarkTreeView::selectionChanged( const QItemSelection & selected, const QItemSelection & deselected )
{
    DEBUG_BLOCK
    Q_UNUSED( deselected )
    QModelIndexList indexes = selected.indexes();
    debug() << indexes.size() << " items selected";
    foreach( QModelIndex index, indexes )
    {
        index = m_proxyModel->mapToSource( index );
        if( index.column() == 0 )
        {
            BookmarkViewItemPtr item = BookmarkModel::instance()->data( index, 0xf00d ).value<BookmarkViewItemPtr>();

            if ( typeid( * item ) == typeid( AmarokUrl ) ) {
                debug() << "a url was selected...";
                AmarokUrl bookmark = *static_cast< AmarokUrl* >( item.data() );
                emit( bookmarkSelected( bookmark ) );
            }
        }
    }
    
}

KMenu* BookmarkTreeView::contextMenu( const QPoint& point )
{
    DEBUG_BLOCK
    KMenu* menu = new KMenu( 0 );

    debug() << "getting menu for point:" << point;
    QModelIndex index = m_proxyModel->mapToSource( indexAt( point ) );
    if( index.isValid() )
    {

        debug() << "got valid index";
        
        QModelIndexList indices = selectionModel()->selectedIndexes();

        QList<KAction *> actions = createCommonActions( indices );

        foreach( KAction * action, actions )
            menu->addAction( action );

        if( indices.count() == 0 )
            menu->addAction( m_addGroupAction );

    }
    
    return menu;
}

void BookmarkTreeView::slotCreateTimecodeTrack() const
{

    //TODO: Factor into separate class
    
    QList<BookmarkViewItemPtr> list = selectedItems().toList();
    if ( list.count() != 2 )
        return;

    const AmarokUrl * url1 = dynamic_cast<const AmarokUrl *>( list.at( 0 ).data() );

    if ( url1 == 0 )
        return;
    if ( url1->command() != "play" )
        return;

    const AmarokUrl * url2 = dynamic_cast<const AmarokUrl *>( list.at( 1 ).data() );

    if ( url2 == 0 )
        return;
    if ( url2->command() != "play" )
        return;

    if ( url1->path() != url2->path() )
        return;

    //ok, so we actually have to timecodes from the same base url, not get the
    //minimum and maximum time:

    int pos1 = 0;
    int pos2 = 0;

    if ( url1->args().keys().contains( "pos" ) )
    {
        pos1 = url1->args().value( "pos" ).toInt();
    }

    if ( url2->args().keys().contains( "pos" ) )
    {
        pos2 = url2->args().value( "pos" ).toInt();
    }

    if ( pos1 == pos2 )
        return;

    int start = qMin( pos1, pos2 ) * 1000;
    int end = qMax( pos1, pos2 ) * 1000;

    //Now we really should pop up a menu to get the user to enter some info about this
    //new track, but for now, just fake it as this is just for testing anyway

    QString url = QUrl::fromEncoded ( QByteArray::fromBase64 ( url1->path().toUtf8() ) ).toString();
    
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
    album->setIsCompilation( false );

    //make the user give us some info about this item...

    Meta::TrackList tl;
    tl.append( Meta::TrackPtr::staticCast( track ) );
    TagDialog *dialog = new TagDialog( tl, 0 );
    dialog->show();


    //now add it to the playlist

    The::playlistController()->insertOptioned( Meta::TrackPtr::staticCast( track ), Playlist::AppendAndPlay );

    
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

#include "BookmarkTreeView.moc"



