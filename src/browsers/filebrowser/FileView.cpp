/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "FileView.h"

#include "Debug.h"
#include "collection/CollectionManager.h"
#include "collection/support/FileCollectionLocation.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "dialogs/TagDialog.h"
#include "DirectoryLoader.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "meta/PlaylistFileSupport.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistModelStack.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"

#include <KIO/DeleteJob>
#include <KDialog>
#include <KDirModel>
#include <KFileItem>
#include <KIcon>
#include <KLocale>
#include <KMenu>
#include <KUrl>

#include <QContextMenuEvent>
#include <QFileSystemModel>
#include <QItemDelegate>
#include <QPainter>



FileView::FileView( QWidget * parent )
    : Amarok::PrettyTreeView( parent )
    , m_appendAction( 0 )
    , m_loadAction( 0 )
    , m_editAction( 0 )
    , m_separator1( 0 )
    , m_deleteAction( 0 )
    , m_pd( 0 )
    , m_ongoingDrag( false )
    , m_moveActivated( false )
    , m_copyActivated( false )
    , m_moveAction( 0 )
    , m_copyAction( 0 )
{
    setFrameStyle( QFrame::NoFrame );
    setItemsExpandable( false );
    setRootIsDecorated( false );
    setAlternatingRowColors( true );

    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );
}

void FileView::contextMenuEvent ( QContextMenuEvent * e )
{

    if( !model() )
        return;

    QModelIndexList indices = selectedIndexes();

    // Abort if nothing is selected
    if( indices.isEmpty() )
        return;

    

    KMenu* menu = new KMenu( this );

    QList<QAction *> actions = actionsForIndices( indices );

    foreach( QAction * action, actions )
        menu->addAction( action );

    // Create Copy/Move to menu items
    // ported from old filebrowser
    QList<Amarok::Collection*> writableCollections;
    QHash<Amarok::Collection*, CollectionManager::CollectionStatus> hash = CollectionManager::instance()->collections();
    QHash<Amarok::Collection*, CollectionManager::CollectionStatus>::const_iterator it = hash.constBegin();
    while ( it != hash.constEnd() )
    {
        Amarok::Collection *coll = it.key();
        if ( coll && coll->isWritable() )
        {
            writableCollections.append( coll );
        }
        ++it;
    }
    if ( !writableCollections.isEmpty() )
    {
        QMenu *moveMenu = new QMenu( i18n( "Move to Collection" ), this );
        foreach( Amarok::Collection *coll, writableCollections )
        {
            CollectionAction *moveAction = new CollectionAction( coll, this );
            connect( moveAction, SIGNAL( triggered() ), this, SLOT( slotPrepareMoveTracks() ) );
            moveMenu->addAction( moveAction );
        }
        menu->addMenu( moveMenu );

        QMenu *copyMenu = new QMenu( i18n( "Copy to Collection" ), this );
        foreach( Amarok::Collection *coll, writableCollections )
        {
            CollectionAction *copyAction = new CollectionAction( coll, this );
            connect( copyAction, SIGNAL( triggered() ), this, SLOT( slotPrepareCopyTracks() ) );
            copyMenu->addAction( copyAction );
        }
        menu->addMenu( copyMenu );
    }

    menu->exec( e->globalPos() );
 
}

void FileView::slotAppendToPlaylist()
{
    addSelectionToPlaylist( false );
}


void FileView::slotReplacePlaylist()
{
    addSelectionToPlaylist( true );
}

void FileView::slotEditTracks()
{
    Meta::TrackList tracks = tracksForEdit();
    if( !tracks.isEmpty() )
    {
        TagDialog *dialog = new TagDialog( tracks, this );
        dialog->show();
    }
}

void FileView::slotPrepareMoveTracks()
{
    if( m_moveActivated )
        return;

    CollectionAction *action = dynamic_cast<CollectionAction*>( sender() );
    if ( !action )
        return;

    m_moveActivated = true;
    m_moveAction = action;

    const KFileItemList list = selectedItems();
    if ( list.isEmpty() )
        return;

    DirectoryLoader* dl = new DirectoryLoader();
    connect( dl, SIGNAL( finished( const Meta::TrackList& ) ), this, SLOT( slotMoveTracks( const Meta::TrackList& ) ) );
    dl->init( list.urlList() );
}

void FileView::slotPrepareCopyTracks()
{
    if( m_copyActivated )
        return;

    CollectionAction *action = dynamic_cast<CollectionAction*>( sender() );
    if ( !action )
        return;

    m_copyActivated = true;
    m_copyAction = action;

    const KFileItemList list = selectedItems();
    if ( list.isEmpty() )
        return;

    DirectoryLoader* dl = new DirectoryLoader();
    connect( dl, SIGNAL( finished( const Meta::TrackList& ) ), this, SLOT( slotCopyTracks( const Meta::TrackList& ) ) );
    dl->init( list.urlList() );
}

void
FileView::slotCopyTracks( const Meta::TrackList& tracks )
{
    if( !m_copyAction || !m_copyActivated )
        return;

    QSet<Amarok::Collection*> collections;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        collections.insert( track->collection() );
    }

    if( collections.count() == 1 )
    {
        Amarok::Collection *sourceCollection = collections.values().first();
        CollectionLocation *source;
        if( sourceCollection )
        {
            source = sourceCollection->location();
        }
        else
        {
            source = new FileCollectionLocation();
        }
        CollectionLocation *destination = m_copyAction->collection()->location();
        source->prepareCopy( tracks, destination );
    }
    else
    {
        warning() << "Cannot handle copying tracks from multiple collections, doing nothing to be safe";
    }
    m_copyActivated = false;
    m_copyAction = 0;
}

void
FileView::slotMoveTracks( const Meta::TrackList& tracks )
{
    if( !m_moveAction || !m_moveActivated )
        return;

    QSet<Amarok::Collection*> collections;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        collections.insert( track->collection() );
    }
    if( collections.count() == 1 )
    {
        Amarok::Collection *sourceCollection = collections.values().first();
        CollectionLocation *source;
        if( sourceCollection )
        {
            source = sourceCollection->location();
        }
        else
        {
            source = new FileCollectionLocation();
        }
        CollectionLocation *destination = m_moveAction->collection()->location();

        source->prepareMove( tracks, destination );
    }
    else
    {
        warning() << "Cannot handle moving tracks from multipe collections, doing nothing to be safe";
    }
    m_moveActivated = false;
    m_moveAction = 0;
}



QList<QAction *> FileView::actionsForIndices( const QModelIndexList &indices )
{
    QList<QAction *> actions;
    
    if( indices.isEmpty() )
        return actions; // get out of here!

    if( m_appendAction == 0 )
    {
        m_appendAction = new QAction( KIcon( "media-track-add-amarok" ), i18n( "&Add to Playlist" ), this );
        m_appendAction->setProperty( "popupdropper_svg_id", "append" );
        connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendToPlaylist() ) );
    }

    actions.append( m_appendAction );

    if( m_loadAction == 0 )
    {
        m_loadAction = new QAction( KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Replace Playlist" ), this );
        m_loadAction->setProperty( "popupdropper_svg_id", "load" );
        connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotReplacePlaylist() ) );
    }

    actions.append( m_loadAction );

    if( m_editAction == 0 )
    {
        m_editAction = new QAction( KIcon( "media-track-edit-amarok" ), i18n( "&Edit Track Details" ), this );
        m_editAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_editAction, SIGNAL( triggered() ), this, SLOT( slotEditTracks() ) );
    }

    actions.append( m_editAction );

    if( m_separator1 == 0 )
    {
            m_separator1 = new QAction( this );
            m_separator1->setSeparator( true );
    }
    
    actions.append( m_separator1 );

    if( m_deleteAction == 0 )
    {
        m_deleteAction = new QAction( KIcon( "media-track-remove-amarok" ), i18n( "&Delete" ), this );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete_file" );
        connect( m_deleteAction, SIGNAL( triggered() ), this, SLOT( slotDelete() ) );
    }

    actions.append( m_deleteAction );

    Meta::TrackList tracks = tracksForEdit();
    m_editAction->setEnabled( !tracks.isEmpty() );

    return actions;
}

void FileView::addSelectionToPlaylist( bool replace )
{
    DEBUG_BLOCK
    QModelIndexList indices = selectedIndexes();

    if( indices.count() == 0 )
        return;
    QList<KUrl> urls;

    foreach( const QModelIndex& index, indices )
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        debug() << "file path: " << file.url();
        if( EngineController::canDecode( file.url() ) || Meta::isPlaylist( file.url() ) || file.isDir() )
        {
            urls << file.url();
        }
    }

    The::playlistController()->insertOptioned( urls, replace ? Playlist::Replace : Playlist::AppendAndPlay );
}


void
FileView::startDrag( Qt::DropActions supportedActions )
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

        QList<QAction *> actions = actionsForIndices( indices );

        QFont font;
        font.setPointSize( 16 );
        font.setBold( true );

        foreach( QAction * action, actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( clear() ) );
        m_pd->hide();
    }

    m_dragMutex.lock();
    m_ongoingDrag = false;
    m_dragMutex.unlock();
}

KFileItemList FileView::selectedItems() const
{
    KFileItemList items;
    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return items;

    foreach( QModelIndex index, indices )
    {
        KFileItem item = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        items << item;
    }
    return items;
}



Meta::TrackList
FileView::tracksForEdit() const
{
    Meta::TrackList tracks;

    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return tracks;

    foreach( QModelIndex index, indices )
    {
        KFileItem item = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( item.url() );
        if( track )
            tracks << track;
    }
    return tracks;
}

void FileView::slotDelete()
{
    DEBUG_BLOCK

    QModelIndexList indices = selectedIndexes();

    if( indices.count() == 0 )
        return;

    KDialog dialog( The::mainWindow() );
    dialog.setCaption( i18n( "Confirm Delete" ) );
    dialog.setButtons( KDialog::Ok | KDialog::Cancel );
    QLabel label( i18np( "Are you sure you want to delete this item?",
                         "Are you sure you want to delete these %1 items?",
                         indices.count() )
                    , &dialog
                  );
    dialog.setButtonText( KDialog::Ok, i18n( "Yes, delete from disk." ) );
    dialog.setMainWidget( &label );
    if( dialog.exec() != QDialog::Accepted )
        return;
    
    
    QList<KUrl> urls;

    foreach( QModelIndex index, indices )
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        debug() << "file path: " << file.url();

        KIO::DeleteJob * job = KIO::del( file.url() );
        job->start();
    }

}

#include "FileView.moc"
