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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistManager.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokconfig.h"
#include "App.h"
#include "statusbar/StatusBar.h"
#include "CollectionManager.h"
#include "PlaylistFileSupport.h"
#include "PodcastProvider.h"
#include "sql/SqlPodcastProvider.h"
#include "sql/SqlUserPlaylistProvider.h"
#include "Debug.h"
#include "MainWindow.h"
#include "meta/M3UPlaylist.h"
#include "meta/PLSPlaylist.h"
#include "meta/XSPFPlaylist.h"
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

    m_defaultPodcastProvider = new SqlPodcastProvider();
    addProvider( m_defaultPodcastProvider, PlaylistManager::PodcastChannel );
    CollectionManager::instance()->addTrackProvider( m_defaultPodcastProvider );

    m_defaultUserPlaylistProvider = new SqlUserPlaylistProvider();
    addProvider( m_defaultUserPlaylistProvider, UserPlaylist );
}

PlaylistManager::~PlaylistManager()
{
    delete m_defaultPodcastProvider;
    delete m_defaultUserPlaylistProvider;
}

bool
PlaylistManager::isPlaylist( const KUrl & path )
{
    const QString ext = Amarok::extension( path.fileName() );

    if( ext == "m3u" ) return true;
    if( ext == "pls" ) return true;
    if( ext == "ram" ) return true;
    if( ext == "smil") return true;
    if( ext == "asx" || ext == "wax" ) return true;
    if( ext == "xml" ) return true;
    if( ext == "xspf" ) return true;

    return false;
}

KUrl
PlaylistManager::newPlaylistFilePath( const QString & fileExtension )
{
    int trailingNumber = 1;
    KLocalizedString fileName = ki18n("Playlist_%1");
    KUrl url( Amarok::saveLocation( "playlists" ) );
    url.addPath( fileName.subs( trailingNumber ).toString() );

    while( QFileInfo( url.path() ).exists() )
        url.setFileName( fileName.subs( ++trailingNumber ).toString() );

    return KUrl( url.path() + fileExtension );
}

void
PlaylistManager::addProvider( PlaylistProvider * provider, int category )
{
    DEBUG_BLOCK

    bool newCategory = false;
    if( !m_map.uniqueKeys().contains( category ) )
            newCategory = true;

    m_map.insert( category, provider );
    connect( provider, SIGNAL(updated()), SLOT(slotUpdated( /*PlaylistProvider **/ )) );

    if( newCategory )
        emit( categoryAdded( category ) );
}

void
PlaylistManager::removeProvider( PlaylistProvider * provider )
{
    DEBUG_BLOCK

    if ( !provider )
        return;

    if ( m_map.values( provider->category() ).contains( provider ) )
    {
        debug() << "Providers of this category: " << providersForCategory( provider->category() ).count();
        debug() << "Removing provider from map";
        int removed = m_map.remove( provider->category(), provider );
        debug() << "Removed provider from map:" << ( m_map.contains( provider->category(), provider ) ? "false" : "true" );
        debug() << "Providers removed: " << removed;
        // Handle deletion of provider here
//        provider->deleteLater();
        slotUpdated();

    }


}

int
PlaylistManager::registerCustomCategory( const QString & name )
{
    int typeNumber = Custom + m_customCategories.size() + 1;

    //TODO: find the name in the configfile, might have been registered before.
    m_customCategories[typeNumber] = name;

    return typeNumber;
}

void
PlaylistManager::slotUpdated( /*PlaylistProvider * provider*/ )
{
    DEBUG_BLOCK
    emit(updated());
}

Meta::PlaylistList
PlaylistManager::playlistsOfCategory( int playlistCategory )
{
    DEBUG_BLOCK
    QList<PlaylistProvider *> providers = m_map.values( playlistCategory );
    QListIterator<PlaylistProvider *> i( providers );

    Meta::PlaylistList list;
    while ( i.hasNext() )
    {
        list << i.next()->playlists();
    }

    return list;
}

PlaylistProviderList
PlaylistManager::providersForCategory( int playlistCategory )
{
    return m_map.values( playlistCategory );
}

PlaylistProvider *
PlaylistManager::playlistProvider(int category, QString name)
{
    QList<PlaylistProvider *> providers( m_map.values( category ) );

    QListIterator<PlaylistProvider *> i(providers);
    while( i.hasNext() )
    {
        PlaylistProvider * p = static_cast<PlaylistProvider *>( i.next() );
        if( p->prettyName() == name )
            return p;
    }

    return 0;
}

void
PlaylistManager::downloadPlaylist( const KUrl & path, const Meta::PlaylistPtr playlist )
{
    DEBUG_BLOCK

    KIO::StoredTransferJob * downloadJob =  KIO::storedGet( path );

    m_downloadJobMap[downloadJob] = playlist;

    connect( downloadJob, SIGNAL( result( KJob * ) ),
             this, SLOT( downloadComplete( KJob * ) ) );

    The::statusBar()->newProgressOperation( downloadJob, i18n( "Downloading Playlist" ) );
}

void
PlaylistManager::downloadComplete( KJob * job )
{
    DEBUG_BLOCK

    if ( !job->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    Meta::PlaylistPtr playlist = m_downloadJobMap.take( job );

    QString contents = static_cast<KIO::StoredTransferJob *>(job)->data();
    QTextStream stream;
    stream.setString( &contents );

    playlist->load( stream );
}

QString
PlaylistManager::typeName( int playlistCategory )
{
    switch( playlistCategory )
    {
        case CurrentPlaylist: return i18n("Current Playlist");
        case UserPlaylist: return i18n("My Playlists");
        case PodcastChannel: return i18n("Podcasts");
        case Dynamic: return i18n("Dynamic Playlists");
        case SmartPlaylist: return i18n("Smart Playlist");
    }
    //if control reaches here playlistCategory is either invalid or a custom category
    if( m_customCategories.contains( playlistCategory ) )
        return m_customCategories[playlistCategory];
    else
        //note: this shouldn't happen so I'm not translating it to facilitate bug reports
        return QString("!!!Invalid Playlist Category!!!\nPlease Report this at bugs.kde.org.");
}

bool
PlaylistManager::save( Meta::TrackList tracks, const QString & name, bool editNow, const QString &fromLocation )
{
    DEBUG_BLOCK
    Q_UNUSED( fromLocation )

    debug() << "Tracks received for saving: " << tracks.count();

    QSet<Amarok::Collection*> collections;

    // Get the collections that the tracks belong to
    foreach( Meta::TrackPtr track, tracks )
    {
        collections << track->collection();
    }

    UserPlaylistProvider *prov = 0;

    // If they don't all belong to one and the same collection,
    // ask the user which provider to save to
    if ( collections.size() > 1 )
    {
        QList<Amarok::Collection*> colls = QList<Amarok::Collection*>::fromSet( collections );

        // TODO: sort the collections by prettyName

        QStringList collList;

        foreach( Amarok::Collection* coll, colls )
            collList << coll->prettyName();

        bool ok = false;

        // Present the dialog to the user

        QString item = KInputDialog::getItem(  i18n(  "Select Collection to Save Playlist" ),  i18n(  "Collections" ),  collList,  0,  false,  &ok,  0 );

        if ( !ok )
            return false;

        prov = colls[ collList.indexOf( item ) ]->userPlaylistProvider();

    }
    // If they all belong to the same collection, save to that
    // collection's provider
    else
    {
        debug() << "All tracks belong to the same collection";
        prov = QList<Amarok::Collection*>::fromSet( collections ).front()->userPlaylistProvider();
    }

    // NOTE: For now, we tell the provider to only save the tracks
    // that correspond to its collection, since that's all the provider
    // should have the ability to save.  Later on, we will give the option
    // of copying tracks belonging to other collections, to this one, in which
    // case we will put ALL the tracks into the playlist

    // find collection associated with the provider we are saving to
    Amarok::Collection *coll;

    foreach( Amarok::Collection *co, collections )
    {
        if( co->userPlaylistProvider() == prov )
        {
            coll = co;
            break;
        }
    }

    // HACK: If no provider available, assume we're using the default sql user playlist provider
    if( !prov )
    {
        debug() << "Provider is null, assuming default playlist provider";
        prov = m_defaultUserPlaylistProvider;
    }

    Meta::TrackList playlistTracks;

    // Filter the tracklist for tracks that belong to the collection
    // whose provider we are saving to
    foreach( const Meta::TrackPtr track, tracks )
    {
        if( track->collection() == coll )
            playlistTracks << track;

    }

    debug() << "Tracks to put in playlist: " << playlistTracks.count();

    Meta::PlaylistPtr playlist = Meta::PlaylistPtr();
    if( name.isEmpty() || editNow )
    {
        debug() << "Empty name of playlist, or editing now";
        playlist = prov->save( playlistTracks );
        AmarokUrl("amarok://navigate/playlists/My Playlists").run();
        emit( renamePlaylist( playlist ) );
    }
    else
    {
        debug() << "Playlist is being saved with name: " << name;
        playlist = prov->save( playlistTracks, name );
    }

    debug() << "Playlist was saved: " << ( playlist.isNull() ? "false" : "true" );

    return !playlist.isNull();
}

bool
PlaylistManager::import( const QString& fromLocation )
{
    DEBUG_BLOCK
    SqlUserPlaylistProvider *sqlProvider =
            dynamic_cast<SqlUserPlaylistProvider *>(m_defaultUserPlaylistProvider);
    if( !sqlProvider )
    {
        debug() << "ERROR: sqlUserPlaylistProvider was null";
        return false;
    }
    return sqlProvider->import( fromLocation );
}

bool
PlaylistManager::exportPlaylist( Meta::TrackList tracks,
                        const QString &location )
{
    DEBUG_BLOCK

    KUrl url( location );
    Meta::Playlist *playlist = 0;

    Meta::Format format = Meta::getFormat( location );
    switch( format )
    {
        case Meta::PLS:
            playlist = new Meta::PLSPlaylist( tracks );
            break;
        case Meta::M3U:
            playlist = new Meta::M3UPlaylist( tracks );
            break;
//         case RAM:
//             playlist = loadRealAudioRam( stream );
//             break;
//         case ASX:
//             playlist = loadASX( stream );
//             break;
//         case SMIL:
//             playlist = loadSMIL( stream );
//             break;
        case Meta::XSPF:
            playlist = new Meta::XSPFPlaylist( tracks );
            break;

        default:
            debug() << "unknown type!";
            break;
    }

    if( !playlist )
        return false;

    playlist->save( location, AmarokConfig::relativePlaylist() );
    delete playlist;

    return true;
}

bool
PlaylistManager::canExpand( Meta::TrackPtr track )
{
    if( !track )
        return false;

    return Meta::getFormat( track->uidUrl() ) != Meta::NotPlaylist;
}

Meta::PlaylistPtr
PlaylistManager::expand( Meta::TrackPtr track )
{
   //this should really be made asyncrhonous
   return Meta::loadPlaylist( track->uidUrl() );
}

PlaylistManager::PlaylistFormat
PlaylistManager::getFormat( const KUrl &path )
{
    const QString ext = Amarok::extension( path.fileName() );

    if( ext == "m3u" ) return M3U;
    if( ext == "pls" ) return PLS;
    if( ext == "ram" ) return RAM;
    if( ext == "smil") return SMIL;
    if( ext == "asx" || ext == "wax" ) return ASX;
    if( ext == "xml" ) return XML;
    if( ext == "xspf" ) return XSPF;

    return Unknown;
}

PlaylistProvider*
PlaylistManager::getProviderForPlaylist( const Meta::PlaylistPtr &playlist )
{
    DEBUG_BLOCK
    // Iteratively check all providers' playlists for ownership
    foreach( PlaylistProvider* provider, m_map.values() )
        {
            debug() << "Checking provider";
            Meta::PlaylistList plistlist = provider->playlists();
            foreach( const Meta::PlaylistPtr plist, plistlist )
                {
                    debug() << "Checking playlist";
                    if ( plist == playlist )
                        return provider;
                }
        }

    debug() << "Returning 0, no matching providers found";

    return 0;
}

QList<PopupDropperAction *>
PlaylistManager::playlistActions( const Meta::PlaylistList playlists )
{
    QList<PopupDropperAction *> actions;
    foreach( const Meta::PlaylistPtr playlist, playlists )
    {
        PlaylistProvider *provider = getProviderForPlaylist( playlist );
        if( provider )
            actions << provider->playlistActions( playlist );
    }

    return actions;
}

QList<PopupDropperAction *>
PlaylistManager::trackActions( const Meta::PlaylistPtr playlist, int trackIndex )
{
    QList<PopupDropperAction *> actions;
    PlaylistProvider *provider = getProviderForPlaylist( playlist );
    if( provider )
        actions << provider->trackActions( playlist, trackIndex );

    return actions;
}

namespace Amarok
{
    //this function (C) Copyright 2003-4 Max Howell, (C) Copyright 2004 Mark Kretschmann
    KUrl::List
    recursiveUrlExpand ( const KUrl &url )
    {
        typedef QMap<QString, KUrl> FileMap;

        KDirLister lister ( false );
        lister.setAutoUpdate ( false );
        lister.setAutoErrorHandlingEnabled ( false, 0 );
        lister.openUrl ( url );

        while ( !lister.isFinished() )
            kapp->processEvents ( QEventLoop::ExcludeUserInputEvents );

        KFileItemList items = lister.items();
        KUrl::List urls;
        FileMap files;
        foreach ( const KFileItem& it, items )
        {
            if ( it.isFile() ) { files[it.name() ] = it.url(); continue; }
            if ( it.isDir() ) urls += recursiveUrlExpand( it.url() );
        }

        oldForeachType ( FileMap, files )
        // users often have playlist files that reflect directories
        // higher up, or stuff in this directory. Don't add them as
        // it produces double entries
        if ( !PlaylistManager::isPlaylist( ( *it ).fileName() ) )
            urls += *it;
        return urls;
    }

    KUrl::List
    recursiveUrlExpand ( const KUrl::List &list )
    {
        KUrl::List urls;
        oldForeachType ( KUrl::List, list )
        {
            urls += recursiveUrlExpand ( *it );
        }

        return urls;
    }
}

#include "PlaylistManager.moc"
