/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "PlaylistManager.h"

#include "amarokconfig.h"
#include "App.h"
#include "StatusBar.h"
#include "CollectionManager.h"
#include "PlaylistFileSupport.h"
#include "PodcastProvider.h"
#include "sql/SqlPodcastProvider.h"
#include "Debug.h"
#include "meta/M3UPlaylist.h"
#include "meta/PLSPlaylist.h"
#include "meta/XSPFPlaylist.h"
#include "meta/SqlPlaylist.h"
#include "browsers/playlistbrowser/UserPlaylistModel.h"
#include "browsers/playlistbrowser/SqlPlaylistGroup.h"

#include <kdirlister.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
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

PlaylistManager::PlaylistManager()
{
    s_instance = this;
    m_defaultPodcastProvider = new SqlPodcastProvider();
    addProvider( m_defaultPodcastProvider, PlaylistManager::PodcastChannel );
    CollectionManager::instance()->addTrackProvider( m_defaultPodcastProvider );
}

PlaylistManager::~PlaylistManager()
{
    delete m_defaultPodcastProvider;
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
    QList<PlaylistProvider *> providers = m_map.values( playlistCategory );
    QListIterator<PlaylistProvider *> i( providers );

    Meta::PlaylistList list;
    while ( i.hasNext() )
        list << i.next()->playlists();

    return list;
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
PlaylistManager::downloadPlaylist( const KUrl & path, Meta::PlaylistPtr playlist )
{
    DEBUG_BLOCK

    KIO::StoredTransferJob * downloadJob =  KIO::storedGet( path );

    m_downloadJobMap[downloadJob] = playlist;

    connect( downloadJob, SIGNAL( result( KJob * ) ),
             this, SLOT( downloadComplete( KJob * ) ) );

    The::statusBar()->newProgressOperation( downloadJob )
            .setDescription( i18n( "Downloading Playlist" ) );
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
PlaylistManager::typeName(int playlistCategory)
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
PlaylistManager::save( Meta::TrackList tracks, const QString & name)
{
    Meta::SqlPlaylist* playlist = new Meta::SqlPlaylist( name, tracks, SqlPlaylistGroupPtr() );
    delete playlist;

    //jolt the playlist browser model to reload....
    //talk about over-coupling... :|
    //PlaylistBrowserNS::UserModel::instance()->reloadFromDb();

    //we should really enter edit mode for the new playlist, but how...
    // NOTE this doesn't really make sense, especially when batch adding
    //      during a collection scan ---lfranchi 9/5/08
    //PlaylistBrowserNS::UserModel::instance()->editPlaylist( newId );

    return true; //FIXME what's this supposed to return?
}

bool
PlaylistManager::save( const QString& fromLocation )
{
    DEBUG_BLOCK
    KUrl url( fromLocation );
    Meta::Playlist* playlist = 0;
    Meta::Format format = Meta::getFormat( fromLocation );
    switch( format )
    {
        case Meta::PLS:
            playlist = new Meta::PLSPlaylist( url );
            break;
        case Meta::M3U:
            playlist = new Meta::M3UPlaylist( url );
            break;
        case Meta::XSPF:
            playlist = new Meta::XSPFPlaylist( url );
            break;

        default:
            debug() << "unknown type!";
            break;
    }
    Meta::TrackList tracks = playlist->tracks();
    QString name = playlist->name().split(".")[0];

    if( tracks.isEmpty() )
        return false;

    save( tracks, name );
    return true;
}

bool
PlaylistManager::exportPlaylist( Meta::TrackList tracks,
                        const QString &location )
{
    DEBUG_BLOCK

    KUrl url( location );
    //TODO: Meta::Format playlistFormat = Meta::getFormat( location );
//     Meta::M3UPlaylistPtr playlist( new Meta::M3UPlaylist( tracks ) );
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
    return true;
}

bool
PlaylistManager::canExpand( Meta::TrackPtr track )
{
    //DEBUG_BLOCK
    //debug() << "name: " <<  track->name();
    //debug() << "url: " << track->url();
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
