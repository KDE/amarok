/*
 * Copyright (C) 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "AlbumsEngine.h"
#include "AlbumsDefs.h"
#include "AlbumItem.h"
#include "TrackItem.h"

#include <capabilities/ActionsCapability.h>
#include <collections/QueryMaker.h>
#include <core/support/Amarok.h>
#include <core-impl/collections/support/CollectionManager.h>
#include <Debug.h>
#include <dialogs/TagDialog.h>
#include <EngineController.h>
#include <playlist/PlaylistController.h>

#include <QMenu>
#include <QRegularExpression>

#include <KLocalizedString>

#include <algorithm>

AlbumsEngine::AlbumsEngine( QObject *parent )
    : QObject( parent )
    , m_lastQueryMaker( nullptr )
    , m_model( new AlbumsModel( this ) )
    , m_proxyModel( new AlbumsProxyModel( this ) )
{
    EngineController* engine = The::engineController();

    connect( engine, &EngineController::trackPlaying,
             this, &AlbumsEngine::slotTrackChanged );
    connect( engine, &EngineController::stopped,
             this, &AlbumsEngine::stopped );
    connect( engine, &EngineController::trackMetadataChanged,
             this, &AlbumsEngine::slotTrackMetadataChanged );

    m_model->setColumnCount( 1 );
    m_proxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_proxyModel->setSortLocaleAware( true );
    m_proxyModel->setDynamicSortFilter( true );
    m_proxyModel->setSourceModel( m_model );
    m_proxyModel->setFilterRole( NameRole );
    updateRecentlyAddedAlbums();
}

void AlbumsEngine::slotTrackMetadataChanged( Meta::TrackPtr track )
{
    if( !track )
        return;
    if( !track->album() || !track->album()->albumArtist() )
    { // No album artist? Show other albums for the artist if available
      // (and if none known, at least the current album if available)
        if( track->artist() && m_artist != track->artist() )
        {
            m_artist = track->artist();
            update();
        }
        return;
    }

    if( track->album()->albumArtist() == m_artist )
        return;

    m_artist = track->album()->albumArtist();
    update();
}

void AlbumsEngine::slotTrackChanged( const Meta::TrackPtr &track )
{
    if( !track || track == m_currentTrack )
        return;

    m_currentTrack = track;
    slotTrackMetadataChanged( track );
}


void AlbumsEngine::stopped()
{
    m_currentTrack.clear();
    m_artist.clear();

    updateRecentlyAddedAlbums();
}

void AlbumsEngine::update()
{
    DEBUG_BLOCK

    // -- search the collection for albums with the same artist
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setAutoDelete( true );
    qm->addFilter( Meta::valArtist, m_artist->name(), true, true );
    qm->setAlbumQueryMode( Collections::QueryMaker::AllAlbums );
    qm->setQueryType( Collections::QueryMaker::Album );

    connect( qm, &Collections::QueryMaker::newAlbumsReady,
             this, &AlbumsEngine::resultReady, Qt::QueuedConnection );

    m_lastQueryMaker = qm;
    qm->run();
}

void AlbumsEngine::updateRecentlyAddedAlbums()
{
    DEBUG_BLOCK

    // Collect data for the recently added albums
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Album );
    qm->excludeFilter( Meta::valAlbum, QString(), true, true );
    qm->orderBy( Meta::valCreateDate, true );
    qm->limitMaxResultSize( Amarok::config(QStringLiteral("Albums Applet")).readEntry("RecentlyAdded", 5) );

    connect( qm, &Collections::QueryMaker::newAlbumsReady,
             this, &AlbumsEngine::resultReady, Qt::QueuedConnection );

    m_lastQueryMaker = qm;
    qm->run();
}

void AlbumsEngine::resultReady( const Meta::AlbumList &albums )
{
    if( sender() != m_lastQueryMaker )
        return;

    m_model->clear();
    m_proxyModel->setMode( m_currentTrack ? AlbumsProxyModel::SortByYear : AlbumsProxyModel::SortByCreateDate );

    // Include currently playing album in results even when album artist is not current artist
    Meta::AlbumList amended;
    if( m_currentTrack && m_currentTrack->album() && std::find_if( albums.cbegin(), albums.cend(),
                        [=](auto a) { return *m_currentTrack->album() == *a; } ) == albums.cend() )
    {
        amended.append( albums );
        amended.append( m_currentTrack->album() );
    }
    for( auto album : ( amended.length() == 0 ? albums : amended ) )
    {
        // do not show all tracks without an album from the collection, this takes ages
        // TODO: show all tracks from this artist that are not part of an album
        if( album->name().isEmpty() )
            continue;

        Meta::TrackList tracks = album->tracks();
        if( tracks.isEmpty() )
            continue;

        AlbumItem *albumItem = new AlbumItem();
        albumItem->setIconSize( 50 );
        albumItem->setAlbum( album );
        albumItem->setShowArtist( !m_currentTrack );

        int numberOfDiscs = 0;
        int childRow = 0;

        std::stable_sort( tracks.begin(), tracks.end(), Meta::Track::lessThan );

        QMultiHash< int, TrackItem* > trackItems; // hash of tracks items for each disc
        for( const auto &track : tracks )
        {
            if( numberOfDiscs < track->discNumber() )
                numberOfDiscs = track->discNumber();

            TrackItem *trackItem = new TrackItem();
            trackItem->setTrack( track );

            // bold the current track to make it more visible
            if( m_currentTrack && *m_currentTrack == *track )
            {
                trackItem->bolden();
            }

            // If compilation and same artist, then highlight, but only if there's a current track
            if( m_currentTrack
                && m_currentTrack->artist() && track->artist()
                && *m_currentTrack->artist() == *track->artist() && album->isCompilation() )
            {
                trackItem->italicise();
            }
            trackItems.insert( track->discNumber(), trackItem );
        }

        for( int i = 0; i <= numberOfDiscs; ++i )
        {
            QList<TrackItem*> items = trackItems.values( i );
            if( !items.isEmpty() )
            {
                const TrackItem *item = items.first();
                QStandardItem *discItem( nullptr );
                if( numberOfDiscs > 1 )
                {
                    discItem = new QStandardItem( i18n("Disc %1", item->track()->discNumber()) );
                    albumItem->setChild( childRow++, discItem );
                    int discChildRow = 0;
                    for( TrackItem *trackItem : items )
                        discItem->setChild( discChildRow++, trackItem );
                }
                else
                {
                    for( TrackItem *trackItem : items )
                        albumItem->setChild( childRow++, trackItem );
                }
            }
        }
        m_model->appendRow( albumItem );
    }

    m_proxyModel->sort( 0 );
}

QString AlbumsEngine::filterPattern() const
{
    return m_proxyModel->filterRegularExpression().pattern();
}

void AlbumsEngine::setFilterPattern( const QString &pattern )
{
    if( m_proxyModel->filterRegularExpression().pattern() == pattern )
        return;

    m_proxyModel->setFilterRegularExpression( QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption) );
    Q_EMIT filterPatternChanged();
}

void AlbumsEngine::clear()
{
    qDeleteAll( m_model->findItems( QLatin1String( "*" ), Qt::MatchWildcard ) );
    m_model->clear();
}

void AlbumsEngine::appendSelected( const QModelIndexList& indexes ) const
{
    Meta::TrackList selected = getSelectedTracks( indexes );
    The::playlistController()->insertOptioned( selected, Playlist::OnAppendToPlaylistAction );
}

void AlbumsEngine::replaceWithSelected( const QModelIndexList& indexes ) const
{
    Meta::TrackList selected = getSelectedTracks( indexes );
    The::playlistController()->insertOptioned( selected, Playlist::OnReplacePlaylistAction );
}

void AlbumsEngine::queueSelected( const QModelIndexList& indexes ) const
{
    Meta::TrackList selected = getSelectedTracks( indexes );
    The::playlistController()->insertOptioned( selected, Playlist::OnQueueToPlaylistAction );
}

void AlbumsEngine::editSelected( const QModelIndexList& indexes ) const
{
    Meta::TrackList selected = getSelectedTracks( indexes );
    if( !selected.isEmpty() )
    {
        TagDialog *dialog = new TagDialog( selected );
        dialog->show();
    }
}

void AlbumsEngine::showContextMenu( const QModelIndexList &indexes, const QModelIndex &mouseOverIndex ) const
{
    if( indexes.isEmpty() || !mouseOverIndex.isValid() )
        return;

    QMenu menu;
    QAction *appendAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-add-amarok") ), i18n( "&Add to Playlist" ), &menu );
    QAction *loadAction   = new QAction( QIcon::fromTheme( QStringLiteral("folder-open") ), i18nc( "Replace the currently loaded tracks with these", "&Replace Playlist" ), &menu );
    QAction *queueAction  = new QAction( QIcon::fromTheme( QStringLiteral("media-track-queue-amarok") ), i18n( "&Queue" ), &menu );
    QAction *editAction   = new QAction( QIcon::fromTheme( QStringLiteral("media-track-edit-amarok") ), i18n( "Edit Track Details" ), &menu );

    menu.addAction( appendAction );
    menu.addAction( loadAction );
    menu.addAction( queueAction );
    menu.addAction( editAction );

    connect( appendAction, &QAction::triggered, this, [this, indexes] () { appendSelected( indexes ); } );
    connect( loadAction, &QAction::triggered, this, [this, indexes] () { replaceWithSelected( indexes ); } );
    connect( queueAction, &QAction::triggered, this, [this, indexes] () { queueSelected( indexes ); } );
    connect( editAction, &QAction::triggered, this, [this, indexes] () { editSelected( indexes ); } );

    QMenu menuCover( i18n( "Album" ), &menu );
    const QStandardItem *item = m_model->itemFromIndex( m_proxyModel->mapToSource( mouseOverIndex ) );
    if( item->type() == AlbumType )
    {
        Meta::AlbumPtr album = static_cast<const AlbumItem*>( item )->album();
        QScopedPointer<Capabilities::ActionsCapability> ac( album->create<Capabilities::ActionsCapability>() );
        if( ac )
        {
            QList<QAction *> actions = ac->actions();
            if( !actions.isEmpty() )
            {
                // ensure that the actions are cleaned up afterwards
                for( QAction *action : actions )
                {
                    if( !action->parent() )
                        action->setParent( &menuCover );
                }

                menuCover.addActions( actions );
                menuCover.setIcon( QIcon::fromTheme( QStringLiteral("filename-album-amarok") ) );
                menu.addMenu( &menuCover );
            }
        }
    }
    menu.exec( QCursor::pos() );
}

QString AlbumsEngine::getSelectedUrlList(const QModelIndexList &indexes) const
{
    const Meta::TrackList list=getSelectedTracks(indexes);
    QString urlList;
    for(const Meta::TrackPtr &t : list)
    {
        urlList += t->playableUrl().toString() + QStringLiteral("\n");
    }
    return urlList;
}

Meta::TrackList AlbumsEngine::getSelectedTracks( const QModelIndexList& indexes ) const
{
    Meta::TrackList selected;

    for( const QModelIndex &index : indexes )
    {
        if( index.isValid() )
        {
            const QModelIndex &srcIndex = m_proxyModel->mapToSource( index );
            const QStandardItem *item = m_model->itemFromIndex( srcIndex );
            if( item->type() == AlbumType )
            {
                selected << static_cast<const AlbumItem*>( item )->album()->tracks();
            }
            else if( item->type() == TrackType )
            {
                selected << static_cast<const TrackItem*>( item )->track();
            }
            else if( m_model->hasChildren( srcIndex ) ) // disc type
            {
                for( int i = m_model->rowCount( srcIndex ) - 1; i >= 0; --i )
                {
                    const QStandardItem *trackItem = m_model->itemFromIndex( m_model->index(i, 0, srcIndex) );
                    selected << static_cast<const TrackItem*>( trackItem )->track();
                }
            }
        }
    }

    return selected;
}
