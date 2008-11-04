/*
    Copyright (c) 2008 Dan Meltzer <hydrogen@notyetimplemented.com>
              (c) 2008 Seb Ruiz <ruiz@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MyDirOperator.h"

#include "Amarok.h"
#include "Debug.h"
#include "DirectoryLoader.h"
#include "MainWindow.h"
#include "collection/support/FileCollectionLocation.h"
#include "collection/CollectionManager.h"
#include "dialogs/TagDialog.h"
#include "playlist/PlaylistController.h"

#include <KActionMenu>
#include <KLocale>
#include <KActionCollection>
#include <KAction>
#include <QAbstractItemView>
#include <QMenu>


MyDirOperator::MyDirOperator( const KUrl &url, QWidget *parent )
        : KDirOperator( url, parent )
        , mCopyActivated ( false )
        , mMoveActivated ( false )
        , mCopyAction( 0 )
        , mMoveAction( 0 )
{
    DEBUG_BLOCK

    MyDirLister *dirlister = new MyDirLister( true );
    dirlister->setMainWindow( The::mainWindow() );
    setDirLister( dirlister );

    setView( KFile::Simple );
    view()->setSelectionMode( QAbstractItemView::ExtendedSelection );
    view()->setContentsMargins( 0, 0, 0, 0 );
    view()->setFrameShape( QFrame::NoFrame );

    connect( this, SIGNAL( fileSelected( const KFileItem& ) ),
             this,   SLOT( fileSelected( const KFileItem& ) ) );

    //FIXME: This signal is only available under KDE4.2 libraries, so remove the ActionCollection hack
    //when we bump kdelibs dep.
    //connect( this, SIGNAL( contextMenuAboutToShow( const KFileItem &item, QMenu *menu ) ),
    //         this,   SLOT( contextMenuAboutToShow( const KFileItem &item, QMenu *menu ) ) );

    //HACK: crafty method to hijack the context menu
    KActionMenu *actionMenu = static_cast<KActionMenu*>( actionCollection()->action( "popupMenu" ) );
    if ( actionMenu )
    {
        KMenu *menu = actionMenu->menu();
        connect( menu, SIGNAL( aboutToShow() ), this, SLOT( aboutToShowContextMenu() ) );
    }
}

MyDirOperator::~MyDirOperator()
{}

void MyDirOperator::fileSelected( const KFileItem & /*file*/ )
{
    slotAppendChildTracks();
    view()->selectionModel()->clear();
}

void MyDirOperator::aboutToShowContextMenu()
{
    DEBUG_BLOCK

    QMenu *menu = dynamic_cast<QMenu*>( sender() );
    if ( !menu )
        return;

    // Remove the "File Properties" action as it makes no sense to us. We'll show our own tag dialog instead.
    foreach( QAction *a, menu->actions() )
        if( a->objectName() == "properties" )
            a->setVisible( false );

    QList<QAction*> actions = createBasicActions();
    foreach( QAction *action, actions )
        menu->addAction( action );

    QList<Collection*> writableCollections;
    QHash<Collection*, CollectionManager::CollectionStatus> hash = CollectionManager::instance()->collections();
    QHash<Collection*, CollectionManager::CollectionStatus>::const_iterator it = hash.constBegin();
    while ( it != hash.constEnd() )
    {
        Collection *coll = it.key();
        if ( coll && coll->isWritable() )
        {
            writableCollections.append( coll );
        }
        ++it;
    }
    if ( !writableCollections.isEmpty() )
    {
        QMenu *moveMenu = new QMenu( i18n( "Move to Collection" ), this );
        foreach( Collection *coll, writableCollections )
        {
            CollectionAction *moveAction = new CollectionAction( coll, this );
            connect( moveAction, SIGNAL( triggered() ), this, SLOT( slotPrepareMoveTracks() ) );
            moveMenu->addAction( moveAction );
        }
        menu->addMenu( moveMenu );

        QMenu *copyMenu = new QMenu( i18n( "Copy to Collection" ), this );
        foreach( Collection *coll, writableCollections )
        {
            CollectionAction *copyAction = new CollectionAction( coll, this );
            connect( copyAction, SIGNAL( triggered() ), this, SLOT( slotPrepareCopyTracks() ) );
            copyMenu->addAction( copyAction );
        }
        menu->addMenu( copyMenu );
    }
}

void
MyDirOperator::slotCopyTracks( const Meta::TrackList& tracks )
{
    if( !mCopyAction ||  !mCopyActivated  )
        return;

    CollectionLocation *source      = new FileCollectionLocation();
    CollectionLocation *destination = mCopyAction->collection()->location();

    source->prepareCopy( tracks, destination );
    mCopyActivated = false;
    mCopyAction = 0;
}

void
MyDirOperator::slotMoveTracks( const Meta::TrackList& tracks )
{
    if( !mMoveAction ||  !mMoveActivated )
        return;

    CollectionLocation *source      = new FileCollectionLocation();
    CollectionLocation *destination = mMoveAction->collection()->location();

    source->prepareMove( tracks, destination );
    mMoveActivated = false;
    mMoveAction = 0;
}

void
MyDirOperator::slotPrepareMoveTracks()
{
    if( mMoveActivated )
        return;

    CollectionAction *action = dynamic_cast<CollectionAction*>( sender() );
    if ( !action )
        return;

    mMoveActivated = true;
    mMoveAction = action;
    
    const KFileItemList list = selectedItems();
    if ( list.isEmpty() )
        return;
    
    DirectoryLoader* dl = new DirectoryLoader();
    connect( dl, SIGNAL( finished( const Meta::TrackList& ) ), this, SLOT( slotMoveTracks( const Meta::TrackList& ) ) );
    dl->init( list.urlList() );
}

void
MyDirOperator::slotPrepareCopyTracks()
{
    if( mCopyActivated )
        return;

    CollectionAction *action = dynamic_cast<CollectionAction*>( sender() );
    if ( !action )
        return;

    mCopyActivated = true;
    mCopyAction = action;
    
    const KFileItemList list = selectedItems();
    if ( list.isEmpty() )
        return;
    
    DirectoryLoader* dl = new DirectoryLoader();
    connect( dl, SIGNAL( finished( const Meta::TrackList& ) ), this, SLOT( slotCopyTracks( const Meta::TrackList& ) ) );
    dl->init( list.urlList() );
}

void MyDirOperator::slotAppendChildTracks()
{
    const KFileItemList list = selectedItems();
    if ( list.isEmpty() )
        return;
    playChildTracks( list, Playlist::AppendAndPlay );
}

void MyDirOperator::slotPlayChildTracks()
{
    const KFileItemList list = selectedItems();
    if ( list.isEmpty() )
        return;
    playChildTracks( list, Playlist::LoadAndPlay );
}

void
MyDirOperator::playChildTracks( const KFileItemList &items, Playlist::AddOptions insertMode )
{
    QList<KUrl> list;
    foreach( KFileItem item, items )
    {
        list.append( item.url() );
    }

    The::playlistController()->insertOptioned( list, insertMode );
}

Meta::TrackList
MyDirOperator::tracksForEdit() const
{
    Meta::TrackList tracks;

    const KFileItemList list = selectedItems();
    if( list.isEmpty() )
        return tracks;

    foreach( KFileItem item, list )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( item.url() );
        if( track )
            tracks << track;
    }
    return tracks;
}

void
MyDirOperator::slotEditTracks()
{
    Meta::TrackList tracks = tracksForEdit();
    if( !tracks.isEmpty() )
    {
        TagDialog *dialog = new TagDialog( tracks, this );
        dialog->show();
    }
}

QList<QAction*>
MyDirOperator::createBasicActions()
{
    QList<QAction*> actions;

    QAction* appendAction = new QAction( KIcon( "media-track-add-amarok" ), i18n( "&Append to Playlist" ), this );
    QAction* loadAction = new QAction( KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), this );
    QAction* editAction = new QAction( KIcon( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), this );

    connect( appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendChildTracks() ) );
    connect( loadAction, SIGNAL( triggered() ), this, SLOT( slotPlayChildTracks() ) );
    connect( editAction, SIGNAL( triggered() ), this, SLOT( slotEditTracks() ) );

    Meta::TrackList tracks = tracksForEdit();
    editAction->setEnabled( !tracks.isEmpty() );

    actions.append( appendAction );
    actions.append( loadAction );
    actions.append( editAction );

    return actions;
}

#include "MyDirOperator.moc"

