/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "dialogs/TagDialog.h"
#include "EngineController.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistModelStack.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"

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
    , m_pd( 0 )
    , m_ongoingDrag( false )
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
        m_loadAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_editAction, SIGNAL( triggered() ), this, SLOT( slotEditTracks() ) );
    }

    actions.append( m_editAction );

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

    foreach( QModelIndex index, indices )
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        debug() << "file path: " << file.url();
        if( EngineController::canDecode( file.url() ) || file.isDir() )
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
#include "FileView.moc"
