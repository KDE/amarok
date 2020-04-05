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

#define DEBUG_PREFIX "FileView"

#include "FileView.h"

#include "EngineController.h"
#include "PaletteHandler.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"
#include "context/ContextView.h"
#include "core/playlists/PlaylistFormat.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/FileCollectionLocation.h"
#include "core-impl/meta/file/File.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core-impl/support/TrackLoader.h"
#include "dialogs/TagDialog.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QFileSystemModel>
#include <QIcon>
#include <QItemDelegate>
#include <QMenu>
#include <QPainter>
#include <QUrl>

#include <KConfigGroup>
#include <KDirModel>
#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KLocalizedString>
#include <KMessageBox>

FileView::FileView( QWidget *parent )
    : Amarok::PrettyTreeView( parent )
    , m_appendAction( 0 )
    , m_loadAction( 0 )
    , m_editAction( 0 )
    , m_moveToTrashAction( 0 )
    , m_deleteAction( 0 )
    , m_pd( 0 )
    , m_ongoingDrag( false )
{
    setFrameStyle( QFrame::NoFrame );
    setItemsExpandable( false );
    setRootIsDecorated( false );
    setAlternatingRowColors( true );
    setUniformRowHeights( true );
    setEditTriggers( EditKeyPressed );

    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), &PaletteHandler::newPalette,
             this, &FileView::newPalette );
}

void
FileView::contextMenuEvent( QContextMenuEvent *e )
{
    if( !model() )
        return;

    //trying to do fancy stuff while showing places only leads to tears!
    if( model()->objectName() == "PLACESMODEL" )
    {
        e->accept();
        return;
    }

    QModelIndexList indices = selectedIndexes();
    // Abort if nothing is selected
    if( indices.isEmpty() )
        return;

    QMenu menu;
    foreach( QAction *action, actionsForIndices( indices, PlaylistAction ) )
        menu.addAction( action );
    menu.addSeparator();

    // Create Copy/Move to menu items
    // ported from old filebrowser
    QList<Collections::Collection*> writableCollections;
    QHash<Collections::Collection*, CollectionManager::CollectionStatus> hash =
            CollectionManager::instance()->collections();
    QHash<Collections::Collection*, CollectionManager::CollectionStatus>::const_iterator it =
            hash.constBegin();
    while( it != hash.constEnd() )
    {
        Collections::Collection *coll = it.key();
        if( coll && coll->isWritable() )
            writableCollections.append( coll );
        ++it;
    }
    if( !writableCollections.isEmpty() )
    {
        QMenu *copyMenu = new QMenu( i18n( ("Copy to Collection") ), &menu );
        copyMenu->setIcon( QIcon::fromTheme( QStringLiteral("edit-copy") ) );
        foreach( Collections::Collection *coll, writableCollections )
        {
            CollectionAction *copyAction = new CollectionAction( coll, &menu );
            connect( copyAction, &QAction::triggered, this, &FileView::slotPrepareCopyTracks );
            copyMenu->addAction( copyAction );
        }
        menu.addMenu( copyMenu );

        QMenu *moveMenu = new QMenu( i18n( "Move to Collection" ), &menu );
        moveMenu->setIcon( QIcon::fromTheme( QStringLiteral("go-jump") ) );
        foreach( Collections::Collection *coll, writableCollections )
        {
            CollectionAction *moveAction = new CollectionAction( coll, &menu );
            connect( moveAction, &QAction::triggered, this, &FileView::slotPrepareMoveTracks );
            moveMenu->addAction( moveAction );
        }
        menu.addMenu( moveMenu );
    }
    foreach( QAction *action, actionsForIndices( indices, OrganizeAction ) )
        menu.addAction( action );
    menu.addSeparator();

    foreach( QAction *action, actionsForIndices( indices, EditAction ) )
        menu.addAction( action );

    menu.exec( e->globalPos() );
}

void
FileView::mouseReleaseEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        PrettyTreeView::mouseReleaseEvent( event );
        return;
    }

    if( state() == QAbstractItemView::NoState && event->button() == Qt::MidButton )
    {
        addIndexToPlaylist( index, Playlist::OnMiddleClickOnSelectedItems );
        event->accept();
        return;
    }

    KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
    if( state() == QAbstractItemView::NoState &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        style()->styleHint( QStyle::SH_ItemView_ActivateItemOnSingleClick, 0, this ) &&
        ( file.isDir() || file.isNull() ) )
    {
        Q_EMIT navigateToDirectory( index );
        event->accept();
        return;
    }

    PrettyTreeView::mouseReleaseEvent( event );
}

void
FileView::mouseDoubleClickEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    // swallow middle-button double-clicks
    if( event->button() == Qt::MidButton )
    {
        event->accept();
        return;
    }

    if( event->button() == Qt::LeftButton )
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        QUrl url = file.url();
        if( !file.isNull() && ( Playlists::isPlaylist( url ) || MetaFile::Track::isTrack( url ) ) )
            addIndexToPlaylist( index, Playlist::OnDoubleClickOnSelectedItems );
        else
            Q_EMIT navigateToDirectory( index );

        event->accept();
        return;
    }

    PrettyTreeView::mouseDoubleClickEvent( event );
}

void
FileView::keyPressEvent( QKeyEvent *event )
{
    QModelIndex index = currentIndex();
    if( !index.isValid() )
        return;

    switch( event->key() )
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        {
            KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
            QUrl url = file.url();
            if( !file.isNull() && ( Playlists::isPlaylist( url ) || MetaFile::Track::isTrack( url ) ) )
                // right, we test the current item, but then add the selection to playlist
                addSelectionToPlaylist( Playlist::OnReturnPressedOnSelectedItems );
            else
                Q_EMIT navigateToDirectory( index );

            return;
        }
        case Qt::Key_Delete:
            slotMoveToTrash( Qt::NoButton, event->modifiers() );
            break;
        case Qt::Key_F5:
            Q_EMIT refreshBrowser();
            break;
        default:
            break;
    }

    QTreeView::keyPressEvent( event );
}

void
FileView::slotAppendToPlaylist()
{
    addSelectionToPlaylist( Playlist::OnAppendToPlaylistAction );
}

void
FileView::slotReplacePlaylist()
{
    addSelectionToPlaylist( Playlist::OnReplacePlaylistAction );
}

void
FileView::slotEditTracks()
{
    Meta::TrackList tracks = tracksForEdit();
    if( !tracks.isEmpty() )
    {
        TagDialog *dialog = new TagDialog( tracks, this );
        dialog->show();
    }
}

void
FileView::slotPrepareMoveTracks()
{
    if( m_moveDestinationCollection )
        return;

    CollectionAction *action = dynamic_cast<CollectionAction*>( sender() );
    if( !action )
        return;

    m_moveDestinationCollection = action->collection();

    const KFileItemList list = selectedItems();
    if( list.isEmpty() )
        return;

    // prevent bug 313003, require full metadata
    TrackLoader* dl = new TrackLoader( TrackLoader::FullMetadataRequired ); // auto-deletes itself
    connect( dl, &TrackLoader::finished, this, &FileView::slotMoveTracks );
    dl->init( list.urlList() );
}

void
FileView::slotPrepareCopyTracks()
{
    if( m_copyDestinationCollection )
        return;

    CollectionAction *action = dynamic_cast<CollectionAction*>( sender() );
    if( !action )
        return;

    m_copyDestinationCollection = action->collection();

    const KFileItemList list = selectedItems();
    if( list.isEmpty() )
        return;

    // prevent bug 313003, require full metadata
    TrackLoader* dl = new TrackLoader( TrackLoader::FullMetadataRequired ); // auto-deletes itself
    connect( dl, &TrackLoader::finished, this, &FileView::slotCopyTracks );
    dl->init( list.urlList() );
}

void
FileView::slotCopyTracks( const Meta::TrackList& tracks )
{
    if( !m_copyDestinationCollection )
        return;

    QSet<Collections::Collection *> collections;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        collections.insert( track->collection() );
    }

    if( collections.count() == 1 )
    {
        Collections::Collection *sourceCollection = collections.values().first();
        Collections::CollectionLocation *source;
        if( sourceCollection )
            source = sourceCollection->location();
        else
            source = new Collections::FileCollectionLocation();

        Collections::CollectionLocation *destination = m_copyDestinationCollection->location();
        source->prepareCopy( tracks, destination );
    }
    else
        warning() << "Cannot handle copying tracks from multiple collections, doing nothing to be safe";

    m_copyDestinationCollection.clear();
}

void
FileView::slotMoveTracks( const Meta::TrackList& tracks )
{
    if( !m_moveDestinationCollection )
        return;

    QSet<Collections::Collection *> collections;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        collections.insert( track->collection() );
    }

    if( collections.count() == 1 )
    {
        Collections::Collection *sourceCollection = collections.values().first();
        Collections::CollectionLocation *source;
        if( sourceCollection )
            source = sourceCollection->location();
        else
            source = new Collections::FileCollectionLocation();

        Collections::CollectionLocation *destination = m_moveDestinationCollection->location();
        source->prepareMove( tracks, destination );
    }
    else
        warning() << "Cannot handle moving tracks from multiple collections, doing nothing to be safe";

    m_moveDestinationCollection.clear();
}

QList<QAction *>
FileView::actionsForIndices( const QModelIndexList &indices, ActionType type )
{
    QList<QAction *> actions;

    if( indices.isEmpty() )
        return actions; // get out of here!

    if( !m_appendAction )
    {
        m_appendAction = new QAction( QIcon::fromTheme( "media-track-add-amarok" ), i18n( "&Add to Playlist" ),
                                      this );
        m_appendAction->setProperty( "popupdropper_svg_id", "append" );
        connect( m_appendAction, &QAction::triggered, this, &FileView::slotAppendToPlaylist );
    }
    if( type & PlaylistAction )
        actions.append( m_appendAction );

    if( !m_loadAction )
    {
        m_loadAction = new QAction( i18nc( "Replace the currently loaded tracks with these",
                                           "&Replace Playlist" ), this );
        m_loadAction->setProperty( "popupdropper_svg_id", "load" );
        connect( m_loadAction, &QAction::triggered, this, &FileView::slotReplacePlaylist );
    }
    if( type & PlaylistAction )
        actions.append( m_loadAction );

    if( !m_moveToTrashAction )
    {
        m_moveToTrashAction = new QAction( QIcon::fromTheme( "user-trash" ), i18n( "&Move to Trash" ), this );
        m_moveToTrashAction->setProperty( "popupdropper_svg_id", "delete_file" );
        // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
        m_moveToTrashAction->setShortcut( Qt::Key_Delete );
        connect( m_moveToTrashAction, &QAction::triggered, this, &FileView::slotMoveToTrashWithoutModifiers );
    }
    if( type & OrganizeAction )
        actions.append( m_moveToTrashAction );

    if( !m_deleteAction )
    {
        m_deleteAction = new QAction( QIcon::fromTheme( "remove-amarok" ), i18n( "&Delete" ), this );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete_file" );
        // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
        m_deleteAction->setShortcut( Qt::SHIFT + Qt::Key_Delete );
        connect( m_deleteAction, &QAction::triggered, this, &FileView::slotDelete );
    }
    if( type & OrganizeAction )
        actions.append( m_deleteAction );

    if( !m_editAction )
    {
        m_editAction = new QAction( QIcon::fromTheme( "media-track-edit-amarok" ),
                                    i18n( "&Edit Track Details" ), this );
        m_editAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_editAction, &QAction::triggered, this, &FileView::slotEditTracks );
    }
    if( type & EditAction )
    {
        actions.append( m_editAction );
        Meta::TrackList tracks = tracksForEdit();
        m_editAction->setVisible( !tracks.isEmpty() );
    }

    return actions;
}

void
FileView::addSelectionToPlaylist( Playlist::AddOptions options )
{
    addIndicesToPlaylist( selectedIndexes(), options );
}

void
FileView::addIndexToPlaylist( const QModelIndex &idx, Playlist::AddOptions options )
{
    addIndicesToPlaylist( QModelIndexList() << idx, options );
}

void
FileView::addIndicesToPlaylist( QModelIndexList indices, Playlist::AddOptions options )
{
    if( indices.isEmpty() )
        return;

    // let tracks & playlists appear in playlist as they are shown in the view:
    qSort( indices );

    QList<QUrl> urls;
    foreach( const QModelIndex &index, indices )
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        QUrl url = file.url();
        if( file.isDir() || Playlists::isPlaylist( url ) || MetaFile::Track::isTrack( url ) )
        {
            urls << file.url();
        }
    }

    The::playlistController()->insertOptioned( urls, options );
}

void
FileView::startDrag( Qt::DropActions supportedActions )
{
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

        foreach( QAction *action, actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );

    if( m_pd )
    {
        connect( m_pd, &PopupDropper::fadeHideFinished, m_pd, &PopupDropper::clear );
        m_pd->hide();
    }

    m_dragMutex.lock();
    m_ongoingDrag = false;
    m_dragMutex.unlock();
}

KFileItemList
FileView::selectedItems() const
{
    KFileItemList items;
    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return items;

    foreach( const QModelIndex& index, indices )
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

    foreach( const QModelIndex &index, indices )
    {
        KFileItem item = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( item.url() );
        if( track )
            tracks << track;
    }
    return tracks;
}

void
FileView::slotMoveToTrash( Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers )
{
    Q_UNUSED( buttons )
    DEBUG_BLOCK

    QModelIndexList indices = selectedIndexes();
    if( indices.isEmpty() )
        return;

    const bool deleting = modifiers.testFlag( Qt::ShiftModifier );
    QString caption;
    QString labelText;
    if( deleting  )
    {
        caption = i18nc( "@title:window", "Confirm Delete" );
        labelText = i18np( "Are you sure you want to delete this item?",
                           "Are you sure you want to delete these %1 items?",
                           indices.count() );
    }
    else
    {
        caption = i18nc( "@title:window", "Confirm Move to Trash" );
        labelText = i18np( "Are you sure you want to move this item to trash?",
                           "Are you sure you want to move these %1 items to trash?",
                           indices.count() );
    }

    QList<QUrl> urls;
    QStringList filepaths;
    foreach( const QModelIndex& index, indices )
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        filepaths << file.localPath();
        urls << file.url();
    }

    KGuiItem confirmButton = deleting ? KStandardGuiItem::del() : KStandardGuiItem::remove();

    if( KMessageBox::warningContinueCancelList( this, labelText, filepaths, caption, confirmButton ) != KMessageBox::Continue )
        return;

    if( deleting )
    {
        KIO::del( urls, KIO::HideProgressInfo );
        return;
    }

    KIO::trash( urls, KIO::HideProgressInfo );
}

void
FileView::slotDelete()
{
    slotMoveToTrash( Qt::NoButton, Qt::ShiftModifier );
}
