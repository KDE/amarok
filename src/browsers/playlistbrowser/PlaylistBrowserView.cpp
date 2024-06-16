/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#define DEBUG_PREFIX "PlaylistBrowserView"

#include "PlaylistBrowserView.h"

#include "MainWindow.h"
#include "PaletteHandler.h"
#include "PopupDropperFactory.h"
#include "SvgHandler.h"
#include "amarokconfig.h"
#include "browsers/playlistbrowser/PlaylistBrowserModel.h"
#include "browsers/playlistbrowser/PlaylistsByProviderProxy.h"
#include "browsers/playlistbrowser/PlaylistsInFoldersProxy.h"
#include "context/ContextView.h"
#include "core/support/Debug.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "playlist/PlaylistModel.h"
#include "playlistmanager/PlaylistManager.h"
#include "widgets/PrettyTreeRoles.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>

#include <KConfigGroup>

#include <algorithm>

using namespace PlaylistBrowserNS;

PlaylistBrowserNS::PlaylistBrowserView::PlaylistBrowserView( QAbstractItemModel *model,
                                                             QWidget *parent )
    : Amarok::PrettyTreeView( parent )
    , m_pd( nullptr )
    , m_ongoingDrag( false )
{
    DEBUG_BLOCK
    setModel( model );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    setDragDropMode( QAbstractItemView::DragDrop );
    setAcceptDrops( true );
    setEditTriggers( QAbstractItemView::EditKeyPressed );
    setMouseTracking( true ); // needed for highlighting provider action icons

    m_createEmptyPlaylistAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-add-amarok") ),
                                               i18n( "Create an Empty Playlist" ), this );
    connect( m_createEmptyPlaylistAction, &QAction::triggered, this, &PlaylistBrowserView::slotCreateEmptyPlaylist );

    m_appendAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-add-amarok") ),
            i18n( "&Add to Playlist" ), this );
    m_appendAction->setProperty( "popupdropper_svg_id", "append" );
    connect( m_appendAction, &QAction::triggered, this, &PlaylistBrowserView::slotAppend );

    m_loadAction = new QAction( QIcon::fromTheme( QStringLiteral("folder-open") ), i18nc( "Replace the currently "
            "loaded tracks with these", "&Replace Playlist" ), this );
    m_loadAction->setProperty( "popupdropper_svg_id", "load" );
    connect( m_loadAction, &QAction::triggered, this, &PlaylistBrowserView::slotLoad );

    m_setNewAction = new QAction( QIcon::fromTheme( QStringLiteral("rating") ), i18nc( "toggle the \"new\" status "
            " of this podcast episode", "&New" ), this );
    m_setNewAction->setProperty( "popupdropper_svg_id", "new" );
    m_setNewAction->setCheckable( true );
    connect( m_setNewAction, &QAction::triggered, this, &PlaylistBrowserView::slotSetNew );

    m_renamePlaylistAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-edit-amarok") ),
            i18n( "&Rename..." ), this );
    m_renamePlaylistAction->setProperty( "popupdropper_svg_id", "edit" );
    // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
    m_renamePlaylistAction->setShortcut( Qt::Key_F2 );
    connect( m_renamePlaylistAction, &QAction::triggered, this, &PlaylistBrowserView::slotRename );

    m_deletePlaylistAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-remove-amarok") ),
            i18n( "&Delete..." ), this );
    m_deletePlaylistAction->setProperty( "popupdropper_svg_id", "delete" );
    // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
    m_deletePlaylistAction->setShortcut( Qt::Key_Delete );
    connect( m_deletePlaylistAction, &QAction::triggered, this, &PlaylistBrowserView::slotDelete );

    m_removeTracksAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-remove-amarok") ),
            QStringLiteral( "<placeholder>" ), this );
    m_removeTracksAction->setProperty( "popupdropper_svg_id", "delete" );
    // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
    m_removeTracksAction->setShortcut( Qt::Key_Delete );
    connect( m_removeTracksAction, &QAction::triggered, this, &PlaylistBrowserView::slotRemoveTracks );

    m_exportAction = new QAction( QIcon::fromTheme( QStringLiteral("document-export-amarok") ),
            i18n( "&Export As..." ), this );
    connect( m_exportAction, &QAction::triggered, this, &PlaylistBrowserView::slotExport );

    m_separatorAction = new QAction( this );
    m_separatorAction->setSeparator( true );
}

void
PlaylistBrowserNS::PlaylistBrowserView::setModel( QAbstractItemModel *model )
{
    if( this->model() )
        disconnect( this->model(), nullptr, this, nullptr );
    Amarok::PrettyTreeView::setModel( model );

    connect( this->model(), SIGNAL(renameIndex(QModelIndex)), SLOT(edit(QModelIndex)) );
}

void
PlaylistBrowserNS::PlaylistBrowserView::mouseReleaseEvent( QMouseEvent *event )
{
    if( m_pd )
    {
        connect( m_pd, &PopupDropper::fadeHideFinished, m_pd, &QObject::deleteLater );
        m_pd->hide();
        m_pd = nullptr;
    }

    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        PrettyTreeView::mouseReleaseEvent( event );
        return;
    }

    if( event->button() == Qt::MidButton )
    {
        insertIntoPlaylist( index, Playlist::OnMiddleClickOnSelectedItems );
        event->accept();
        return;
    }

    PrettyTreeView::mouseReleaseEvent( event );
}

void PlaylistBrowserNS::PlaylistBrowserView::startDrag( Qt::DropActions supportedActions )
{
    // Waah? when a parent item is dragged, startDrag is called a bunch of times
    if( m_ongoingDrag )
        return;
    m_ongoingDrag = true;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {
        QActionList actions = actionsFor( selectedIndexes() );
        for( QAction *action : actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );

    // We keep the items that the actions need to be applied to.
    // Clear the data from all actions now that the PUD has executed.
    resetActionTargets();

    if( m_pd )
    {
        connect( m_pd, &PopupDropper::fadeHideFinished, m_pd, &PopupDropper::clear );
        m_pd->hide();
    }
    m_ongoingDrag = false;
}

void
PlaylistBrowserNS::PlaylistBrowserView::keyPressEvent( QKeyEvent *event )
{
    QModelIndexList indices = selectedIndexes();
    // mind bug 305203
    if( indices.isEmpty() || state() != QAbstractItemView::NoState )
    {
        Amarok::PrettyTreeView::keyPressEvent( event );
        return;
    }

    switch( event->key() )
    {
        //activated() only works for current index, not all selected
        case Qt::Key_Enter:
        case Qt::Key_Return:
            insertIntoPlaylist( indices, Playlist::OnReturnPressedOnSelectedItems );
            return;
        case Qt::Key_Delete:
        {
            QActionList actions = actionsFor( indices ); // sets action targets
            if( actions.contains( m_removeTracksAction ) )
                m_removeTracksAction->trigger();
            else if( actions.contains( m_deletePlaylistAction ) )
                m_deletePlaylistAction->trigger();
            resetActionTargets();
            return;
        }
        default:
            break;
    }
    Amarok::PrettyTreeView::keyPressEvent( event );
}

void
PlaylistBrowserNS::PlaylistBrowserView::mouseDoubleClickEvent( QMouseEvent *event )
{
    if( event->button() == Qt::MidButton )
    {
        event->accept();
        return;
    }

    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
    {
        event->accept();
        return;
    }

    // code copied in CollectionTreeView::mouseDoubleClickEvent(), keep in sync
    // mind bug 279513
    bool isExpandable = model()->hasChildren( index );
    bool wouldExpand = !visualRect( index ).contains( event->pos() ) || // clicked outside item, perhaps on expander icon
                       ( isExpandable && !style()->styleHint( QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, this ) ); // we're in doubleClick
    if( event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        !wouldExpand )
    {
        insertIntoPlaylist( index, Playlist::OnDoubleClickOnSelectedItems );
        event->accept();
        return;
    }

    PrettyTreeView::mouseDoubleClickEvent( event );
}

void PlaylistBrowserNS::PlaylistBrowserView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex clickedIdx = indexAt( event->pos() );

    QModelIndexList indices;
    if( clickedIdx.isValid() && selectedIndexes().contains( clickedIdx ) )
        indices << selectedIndexes();
    else if( clickedIdx.isValid() )
        indices << clickedIdx;

    QActionList actions = actionsFor( indices );
    if( actions.isEmpty() )
    {
        resetActionTargets();
        return;
    }

    QMenu menu;
    for( QAction *action : actions )
        menu.addAction( action );
    menu.exec( mapToGlobal( event->pos() ) );

    // We keep the items that the action need to be applied to.
    // Clear the data from all actions now that the context menu has executed.
    resetActionTargets();
}

QList<QAction *>
PlaylistBrowserNS::PlaylistBrowserView::actionsFor( const QModelIndexList &indexes )
{
    resetActionTargets();
    if( indexes.isEmpty() )
        return QActionList();

    using namespace Playlists;
    QSet<PlaylistProvider *> providers, writableProviders;
    QActionList actions;
    QModelIndexList newPodcastEpisodes, oldPodcastEpisodes;
    for( const QModelIndex &idx : indexes )
    {
        // direct provider actions:
        actions << idx.data( PrettyTreeRoles::DecoratorRole ).value<QActionList>();

        PlaylistProvider *provider = idx.data( PlaylistBrowserModel::ProviderRole ).value<PlaylistProvider *>();
        if( provider )
            providers << provider;
        bool isWritable =  provider ? provider->isWritable() : false;
        if( isWritable )
            writableProviders |= provider;
        Meta::TrackPtr track = idx.data( PlaylistBrowserModel::TrackRole ).value<Meta::TrackPtr>();
        PlaylistPtr playlist = idx.data( PlaylistBrowserModel::PlaylistRole ).value<PlaylistPtr>();
        if( !track && playlist ) // a playlist (must check it is not a track)
        {
            m_actionPlaylists << playlist;
            if( isWritable )
                m_writableActionPlaylists << playlist;
        }
        if( track )
        {
            m_actionTracks.insert( playlist, idx.row() );
            if( isWritable )
                m_writableActionTracks.insert( playlist, idx.row() );
        }

        QVariant episodeIsNew = idx.data( PlaylistBrowserModel::EpisodeIsNewRole );
        if( episodeIsNew.type() == QVariant::Bool )
        {
            if( episodeIsNew.toBool() )
                newPodcastEpisodes << idx;
            else
                oldPodcastEpisodes << idx;
        }
    }
    // all actions taking provider have only sense with one provider
    if( writableProviders.count() == 1 )
        m_writableActionProvider = writableProviders.values().first();

    // process per-provider actions
    for( PlaylistProvider *provider : providers )
    {
        // prepare arguments and get relevant actions
        PlaylistList providerPlaylists;
        for( const PlaylistPtr &playlist : m_actionPlaylists )
        {
            if( playlist->provider() == provider )
                providerPlaylists << playlist;
        }
        actions << provider->playlistActions( providerPlaylists );

        QMultiHash<PlaylistPtr, int> playlistTracks;
        QHashIterator<PlaylistPtr, int> it( m_actionTracks );
        while( it.hasNext() )
        {
            it.next();
            if( it.key()->provider() == provider )
                playlistTracks.insert( it.key(), it.value() );
        }
        actions << provider->trackActions( playlistTracks );
    }

    // separate model actions from standard actions we provide (at the top)
    QActionList standardActions;
    if( m_actionPlaylists.isEmpty() && m_actionTracks.isEmpty() && m_writableActionProvider )
        standardActions << m_createEmptyPlaylistAction;
    if( !m_actionPlaylists.isEmpty() || !m_actionTracks.isEmpty() )
        standardActions << m_appendAction << m_loadAction;
    if( !newPodcastEpisodes.isEmpty() || !oldPodcastEpisodes.isEmpty() )
    {
        m_setNewAction->setChecked( oldPodcastEpisodes.isEmpty() );
        m_setNewAction->setData( QVariant::fromValue( newPodcastEpisodes + oldPodcastEpisodes ) );
        standardActions << m_setNewAction;
    }
    if( m_writableActionPlaylists.count() == 1 && m_actionTracks.isEmpty() )
        standardActions << m_renamePlaylistAction;
    if( !m_writableActionPlaylists.isEmpty() && m_actionTracks.isEmpty() )
        standardActions << m_deletePlaylistAction;
    if( m_actionPlaylists.isEmpty() && !m_writableActionTracks.isEmpty() )
    {
        const int actionTrackCount = m_writableActionTracks.count();
        const int playlistCount = m_writableActionTracks.uniqueKeys().count();
        if( playlistCount > 1 )
            m_removeTracksAction->setText( i18nc( "%1: number of tracks. %2: number of playlists",
                "Remove %1 From %2", i18ncp ("First part of 'Remove %1 From %2'", "a Track",
                "%1 Tracks", actionTrackCount), i18ncp ("Second part of 'Remove %1 From %2'", "1 Playlist",
                "%1 Playlists", playlistCount ) ) );
        else
            m_removeTracksAction->setText( i18ncp( "%2 is saved playlist name",
                "Remove a Track From %2", "Remove %1 Tracks From %2", actionTrackCount,
                m_writableActionTracks.uniqueKeys().first()->prettyName() ) );
        standardActions << m_removeTracksAction;
    }
    if( m_actionPlaylists.count() == 1 && m_actionTracks.isEmpty() )
        standardActions << m_exportAction;
    standardActions << m_separatorAction;

    return standardActions + actions;
}

void
PlaylistBrowserView::resetActionTargets()
{
    m_writableActionProvider = nullptr;
    m_actionPlaylists.clear();
    m_writableActionPlaylists.clear();
    m_actionTracks.clear();
    m_writableActionTracks.clear();
}

void
PlaylistBrowserNS::PlaylistBrowserView::currentChanged( const QModelIndex &current,
                                                        const QModelIndex &previous )
{
    Q_UNUSED( previous )
    Q_EMIT currentItemChanged( current );
    Amarok::PrettyTreeView::currentChanged( current, previous );
}

void
PlaylistBrowserView::slotCreateEmptyPlaylist()
{
    // m_actionProvider may be null, which is fine
    The::playlistManager()->save( Meta::TrackList(), Amarok::generatePlaylistName(
            Meta::TrackList() ), m_writableActionProvider );
}

void
PlaylistBrowserView::slotAppend()
{
    insertIntoPlaylist( Playlist::OnAppendToPlaylistAction );
}

void
PlaylistBrowserView::slotLoad()
{
    insertIntoPlaylist( Playlist::OnReplacePlaylistAction );
}

void
PlaylistBrowserView::slotSetNew( bool newState )
{
    QModelIndexList indices = m_setNewAction->data().value<QModelIndexList>();
    for( const QModelIndex &idx : indices )
        model()->setData( idx, newState, PlaylistBrowserModel::EpisodeIsNewRole );
}

void
PlaylistBrowserView::slotRename()
{
    if( m_writableActionPlaylists.count() != 1 )
    {
        warning() << __PRETTY_FUNCTION__ << "m_writableActionPlaylists.count() is not 1";
        return;
    }
    Playlists::PlaylistPtr playlist = m_writableActionPlaylists.at( 0 );

    // TODO: this makes a rather complicated round-trip and ends up in edit(QModelIndex)
    // here -- simplify that
    The::playlistManager()->rename( playlist );
}

void
PlaylistBrowserView::slotDelete()
{
    if( m_writableActionPlaylists.isEmpty() )
        return;

    using namespace Playlists;
    QHash<PlaylistProvider *, PlaylistList> providerPlaylists;
    for( const PlaylistPtr &playlist : m_writableActionPlaylists )
    {
        if( playlist->provider() )
            providerPlaylists[ playlist->provider() ] << playlist;
    }
    QStringList providerNames;
    for( const PlaylistProvider *provider : providerPlaylists.keys() )
        providerNames << provider->prettyName();

    QString deletionString = ( m_writableActionPlaylists.count() == 1 ?
        i18nc( "Playlist deletion confirmation dialog. %1 is playlist name, %2 is playlist provider pretty name", "Delete playlist <i>%1</i> from %2?",
               m_writableActionPlaylists.first()->prettyName(), providerNames.join( QStringLiteral(", ") ) ) :
        i18nc( "Playlist deletion confirmation dialog. %1 is the playlist count (always more than 1), %2 is playlist provider pretty name", "Delete %1 playlists from %2?",
               m_writableActionPlaylists.count(), providerNames.join( QStringLiteral(", ") ) ) );

    auto button = QMessageBox::question( The::mainWindow(),
                                         i18n( "Confirm Playlist Deletion" ),
                                         deletionString,
                                         QMessageBox::Yes | QMessageBox::No,
                                         QMessageBox::Yes );

    if( button == QMessageBox::Yes )
    {
        for( PlaylistProvider *provider : providerPlaylists.keys() )
            provider->deletePlaylists( providerPlaylists.value( provider ) );
    }
}

void
PlaylistBrowserView::slotRemoveTracks()
{
    for( Playlists::PlaylistPtr playlist : m_writableActionTracks.uniqueKeys() )
    {
        QList<int> trackIndices = m_writableActionTracks.values( playlist );
        std::sort( trackIndices.begin(), trackIndices.end() );
        int removed = 0;
        for( int trackIndex : trackIndices )
        {
            playlist->removeTrack( trackIndex - removed /* account for already removed */ );
            removed++;
        }
    }
}

void
PlaylistBrowserView::slotExport()
{
    if( m_actionPlaylists.count() != 1 )
    {
        warning() << __PRETTY_FUNCTION__ << "m_actionPlaylists.count() is not 1";
        return;
    }
    Playlists::PlaylistPtr playlist = m_actionPlaylists.at( 0 );

    // --- display save location dialog
    // compare with MainWindow::exportPlaylist
    // TODO: have this code only once
    QFileDialog fileDialog;
    fileDialog.restoreState( Amarok::config( QStringLiteral("playlist-export-dialog") ).readEntry( "state", QByteArray() ) );

    // FIXME: Make checkbox visible in dialog
    QCheckBox *saveRelativeCheck = new QCheckBox( i18n("Use relative path for &saving"), &fileDialog );
    saveRelativeCheck->setChecked( AmarokConfig::relativePlaylist() );

    QStringList supportedMimeTypes;

    supportedMimeTypes << QStringLiteral("video/x-ms-asf"); //ASX
    supportedMimeTypes << QStringLiteral("audio/x-mpegurl"); //M3U
    supportedMimeTypes << QStringLiteral("audio/x-scpls"); //PLS
    supportedMimeTypes << QStringLiteral("application/xspf+xml"); //XSPF

    fileDialog.setMimeTypeFilters( supportedMimeTypes );
    fileDialog.setAcceptMode( QFileDialog::AcceptSave );
    fileDialog.setFileMode( QFileDialog::AnyFile );
    fileDialog.setWindowTitle( i18n("Save As") );
    fileDialog.setObjectName( QStringLiteral("PlaylistExport") );

    int result = fileDialog.exec();
    QString playlistPath = fileDialog.selectedFiles().value( 0 );
    if( result == QDialog::Accepted && !playlistPath.isEmpty() )
        Playlists::exportPlaylistFile( playlist->tracks(), QUrl::fromLocalFile( playlistPath ) );

    Amarok::config( QStringLiteral("playlist-export-dialog") ).writeEntry( "state", fileDialog.saveState() );
}

void
PlaylistBrowserView::insertIntoPlaylist( const QModelIndex &index, Playlist::AddOptions options )
{
    insertIntoPlaylist( QModelIndexList() << index, options );
}

void
PlaylistBrowserView::insertIntoPlaylist( const QModelIndexList &list, Playlist::AddOptions options )
{
    actionsFor( list ); // sets action targets
    insertIntoPlaylist( options );
    resetActionTargets();
}

void
PlaylistBrowserView::insertIntoPlaylist( Playlist::AddOptions options )
{
    Meta::TrackList tracks;

    // add tracks for fully-selected playlists:
    for( Playlists::PlaylistPtr playlist : m_actionPlaylists )
    {
        tracks << playlist->tracks();
    }

    // filter-out tracks from playlists that are selected, add lone tracks:
    for( Playlists::PlaylistPtr playlist : m_actionTracks.uniqueKeys() )
    {
        if( m_actionPlaylists.contains( playlist ) )
            continue;

        Meta::TrackList playlistTracks = playlist->tracks();
        QList<int> positions = m_actionTracks.values( playlist );
        std::sort( positions.begin(), positions.end() );
        for( int position : positions )
        {
            if( position >= 0 && position < playlistTracks.count() )
                tracks << playlistTracks.at( position );
        }
    }

    if( !tracks.isEmpty() )
        The::playlistController()->insertOptioned( tracks, options );
}
