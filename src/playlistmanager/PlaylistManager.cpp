/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistManager.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokconfig.h"
#include "App.h"
#include "statusbar/StatusBar.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/PlaylistFile.h"
#include "playlist/PlaylistModelStack.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core/podcasts/PodcastProvider.h"
#include "file/PlaylistFileProvider.h"
#include "core-impl/podcasts/sql/SqlPodcastProvider.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"
#include "core/support/Debug.h"
#include "MainWindow.h"
#include "browsers/playlistbrowser/UserPlaylistModel.h"

#include <kdirlister.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KInputDialog>
#include <KLocale>
#include <KUrl>

#include <QFileInfo>

PlaylistManager *PlaylistManager::s_instance = 0;

namespace The
{
    PlaylistManager *playlistManager() { return PlaylistManager::instance(); }
}

PlaylistManager *
PlaylistManager::instance()
{
    return s_instance ? s_instance : new PlaylistManager();
}

void
PlaylistManager::destroy()
{
    if (s_instance) {
        delete s_instance;
        s_instance = 0;
    }
}

PlaylistManager::PlaylistManager()
{
    s_instance = this;

    m_defaultPodcastProvider = new Podcasts::SqlPodcastProvider();
    addProvider( m_defaultPodcastProvider, PlaylistManager::PodcastChannel );
    CollectionManager::instance()->addTrackProvider( m_defaultPodcastProvider );

    m_defaultUserPlaylistProvider = new Playlists::SqlUserPlaylistProvider();
    addProvider( m_defaultUserPlaylistProvider, UserPlaylist );

    m_playlistFileProvider = new Playlists::PlaylistFileProvider();
    addProvider( m_playlistFileProvider, UserPlaylist );
}

PlaylistManager::~PlaylistManager()
{
    delete m_defaultPodcastProvider;
    delete m_defaultUserPlaylistProvider;
    delete m_playlistFileProvider;
}

void
PlaylistManager::addProvider( Playlists::PlaylistProvider * provider, int category )
{
    bool newCategory = false;
    if( !m_providerMap.uniqueKeys().contains( category ) )
            newCategory = true;

    m_providerMap.insert( category, provider );
    connect( provider, SIGNAL(updated()), SLOT(slotUpdated( /*PlaylistProvider **/ )) );

    if( newCategory )
        emit( categoryAdded( category ) );

    emit( providerAdded( provider, category ) );
    emit( updated() );
}

void
PlaylistManager::removeProvider( Playlists::PlaylistProvider *provider )
{
    if( !provider )
        return;

    if( m_providerMap.values( provider->category() ).contains( provider ) )
    {
        debug() << "Providers of this category: " << providersForCategory( provider->category() ).count();
        debug() << "Removing provider from map";
        int removed = m_providerMap.remove( provider->category(), provider );
        debug() << "Removed provider from map:" << ( m_providerMap.contains( provider->category(), provider ) ? "false" : "true" );
        debug() << "Providers removed: " << removed;

        emit( providerRemoved( provider, provider->category() ) );

        slotUpdated();
    }
}

void
PlaylistManager::slotUpdated( /*PlaylistProvider * provider*/ )
{
    emit( updated() );
}

Playlists::PlaylistList
PlaylistManager::playlistsOfCategory( int playlistCategory )
{
    QList<Playlists::PlaylistProvider *> providers = m_providerMap.values( playlistCategory );
    QListIterator<Playlists::PlaylistProvider *> i( providers );

    Playlists::PlaylistList list;
    while( i.hasNext() )
        list << i.next()->playlists();

    return list;
}

PlaylistProviderList
PlaylistManager::providersForCategory( int playlistCategory )
{
    return m_providerMap.values( playlistCategory );
}

Playlists::PlaylistProvider *
PlaylistManager::playlistProvider(int category, QString name)
{
    QList<Playlists::PlaylistProvider *> providers( m_providerMap.values( category ) );

    QListIterator<Playlists::PlaylistProvider *> i(providers);
    while( i.hasNext() )
    {
        Playlists::PlaylistProvider * p = static_cast<Playlists::PlaylistProvider *>( i.next() );
        if( p->prettyName() == name )
            return p;
    }

    return 0;
}

void
PlaylistManager::downloadPlaylist( const KUrl &path, const Playlists::PlaylistFilePtr playlist )
{
    KIO::StoredTransferJob *downloadJob =  KIO::storedGet( path );

    m_downloadJobMap[downloadJob] = playlist;

    connect( downloadJob, SIGNAL( result( KJob * ) ),
             this, SLOT( downloadComplete( KJob * ) ) );

    The::statusBar()->newProgressOperation( downloadJob, i18n( "Downloading Playlist" ) );
}

void
PlaylistManager::downloadComplete( KJob *job )
{
    if( !job->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    Playlists::PlaylistFilePtr playlist = m_downloadJobMap.take( job );

    QString contents = static_cast<KIO::StoredTransferJob *>(job)->data();
    QTextStream stream;
    stream.setString( &contents );

    playlist->load( stream );
}

bool
PlaylistManager::save( Meta::TrackList tracks, const QString &name,
                       Playlists::UserPlaylistProvider *toProvider )
{
    AMAROK_DEPRECATED
    // used by: Playlist::Widget::slotSaveCurrentPlaylist()
    //if toProvider is 0 use the default Playlists::UserPlaylistProvider (SQL)
    Playlists::UserPlaylistProvider *prov = toProvider ? toProvider : m_defaultUserPlaylistProvider;
    Playlists::PlaylistPtr playlist = Playlists::PlaylistPtr();
    if( name.isEmpty() )
    {
        debug() << "Empty name of playlist, or editing now";
        playlist = prov->save( tracks );
        if( playlist.isNull() )
            return false;

        AmarokUrl("amarok://navigate/playlists/user playlists").run();
        emit( renamePlaylist( playlist ) );
    }
    else
    {
        debug() << "Playlist is being saved with name: " << name;
        playlist = prov->save( tracks, name );
    }

    return !playlist.isNull();
}

bool
PlaylistManager::import( const QString& fromLocation )
{
    // used by: PlaylistBrowserNS::UserModel::dropMimeData()
    AMAROK_DEPRECATED
    DEBUG_BLOCK
    if( !m_playlistFileProvider )
    {
        debug() << "ERROR: m_playlistFileProvider was null";
        return false;
    }
    return m_playlistFileProvider->import( KUrl(fromLocation) );
}

void
PlaylistManager::rename( Playlists::PlaylistPtr playlist )
{
    if( playlist.isNull() )
        return;

    Playlists::UserPlaylistProvider *provider
            = qobject_cast<Playlists::UserPlaylistProvider *>( getProviderForPlaylist( playlist ) );

    if( !provider )
        return;

    bool ok;
    const QString newName = KInputDialog::getText( i18n("Change playlist"),
                i18n("Enter new name for playlist:"), playlist->name(),
                                                   &ok );
    if( ok )
    {
        debug() << "Changing name from " << playlist->name() << " to " << newName.trimmed();
        provider->rename( playlist, newName.trimmed() );
        emit( updated() );
    }
}

void
PlaylistManager::deletePlaylists( Playlists::PlaylistList playlistlist )
{
    // Map the playlists to their respective providers
    QHash<Playlists::UserPlaylistProvider*, Playlists::PlaylistList> provLists;
    foreach( Playlists::PlaylistPtr playlist, playlistlist )
    {
        // Get the providers of the respective playlists
        Playlists::UserPlaylistProvider *prov = qobject_cast<Playlists::UserPlaylistProvider *>( getProviderForPlaylist( playlist ) );

        if( prov )
        {
            Playlists::PlaylistList pllist;
            pllist << playlist;
            // If the provider already has at least one playlist to delete, add another to its list
            if( provLists.contains( prov ) )
            {
                provLists[ prov ] << pllist;

            }
            // If we are adding a new provider, put it in the hash, initialize its list
            else
                provLists.insert( prov, pllist );
        }
    }

    // Pass each list of playlists to the respective provider for deletion

    foreach( Playlists::UserPlaylistProvider* prov, provLists.keys() )
    {
        prov->deletePlaylists( provLists.value( prov ) ); //TODO: test
    }
}

Playlists::PlaylistProvider*
PlaylistManager::getProviderForPlaylist( const Playlists::PlaylistPtr playlist )
{
    if( !playlist )
        return 0;

    Playlists::PlaylistProvider* provider = playlist->provider();
    if( provider )
        return provider;

    // Iteratively check all providers' playlists for ownership
    QList< Playlists::PlaylistProvider* > userPlaylists = m_providerMap.values( UserPlaylist );
    foreach( Playlists::PlaylistProvider* provider, userPlaylists )
    {
        if( provider->playlists().contains( playlist ) )
                return provider;
    }
    return 0;
}


bool
PlaylistManager::isWritable( const Playlists::PlaylistPtr &playlist )
{
    Playlists::UserPlaylistProvider *provider
            = qobject_cast<Playlists::UserPlaylistProvider *>( getProviderForPlaylist( playlist ) );

    if( provider )
        return provider->isWritable();
    else
        return false;
}

QList<QAction *>
PlaylistManager::playlistActions( const Playlists::PlaylistList playlists )
{
    QList<QAction *> actions;
    foreach( const Playlists::PlaylistPtr playlist, playlists )
    {
        Playlists::PlaylistProvider *provider = getProviderForPlaylist( playlist );
        if( !provider )
            continue;

        //only add actions that are not in the list yet.
        QList<QAction *> playlistActions( provider->playlistActions( playlist ) );
        foreach( QAction *action, playlistActions )
        {
            if( !actions.contains( action ) )
                actions << action;
        }
    }

    return actions;
}

QList<QAction *>
PlaylistManager::trackActions( const Playlists::PlaylistPtr playlist, int trackIndex )
{
    QList<QAction *> actions;
    Playlists::PlaylistProvider *provider = getProviderForPlaylist( playlist );
    if( provider )
        actions << provider->trackActions( playlist, trackIndex );

    return actions;
}

void
PlaylistManager::completePodcastDownloads()
{
    foreach( Playlists::PlaylistProvider *prov, providersForCategory( PodcastChannel ) )
    {
        Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>( prov );
        if( !podcastProvider )
            continue;

        podcastProvider->completePodcastDownloads();
    }
}

#include "PlaylistManager.moc"
