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

#include "App.h"
#include "amarokconfig.h"
#include "PlaylistFileProvider.h"
#include "PlaylistFileSupport.h"
#include "EditablePlaylistCapability.h"
#include "Amarok.h"
#include "Debug.h"
#include "meta/M3UPlaylist.h"
#include "meta/PLSPlaylist.h"
#include "meta/XSPFPlaylist.h"
#include "StatusBar.h"

#include <QString>

#include <kdirlister.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KInputDialog>
#include <KLocale>
#include <KUrl>
#include <KTemporaryFile>

PlaylistFileProvider::PlaylistFileProvider()
 : UserPlaylistProvider()
 , m_defaultFormat( Meta::XSPF )
{
    DEBUG_BLOCK
    //load the playlists defined in the config
    QStringList keys = Amarok::config( "Loaded Playlist Files" ).keyList();
    debug() << "keys " << keys;

    //ConfigEntry: name, file
    foreach( const QString &key, keys )
    {
        QStringList configEntry =
                Amarok::config( "Loaded Playlist Files" ).readXdgListEntry( key );
        Meta::PlaylistPtr playlist = Meta::loadPlaylist( KUrl( configEntry[1] ).path() );
        //TODO: make this work
        if( playlist->is<Meta::EditablePlaylistCapability>() )
        {
            QString title = configEntry[0];
            playlist->create<Meta::EditablePlaylistCapability>()->setTitle( title );
        }
        m_playlists << playlist;
    }
    if( m_playlists.isEmpty() )
    {
        //TODO: find playlist files in the configured collection directories and home folder
    }
}

PlaylistFileProvider::~PlaylistFileProvider()
{
    DEBUG_BLOCK
    //Write loaded playlists to config file
    debug() << m_playlists.size()  << " Playlists loaded";
    int i = 0;
    foreach( Meta::PlaylistPtr playlist, m_playlists )
    {
        QStringList configEntry;
        KUrl url = playlist->retrievableUrl();
        debug() << "storing: " << playlist->name() << " : " << url.url();

        configEntry << playlist->name();
        configEntry << url.url();

        Amarok::config( "Loaded Playlist Files" ).writeXdgListEntry(
                        QString("Playlist %1").arg( ++i ), configEntry );
    }
    Amarok::config( "Loaded Playlist Files" ).sync();
}

QString
PlaylistFileProvider::prettyName() const
{
    return i18n("Playlist Files");
}

Meta::PlaylistList
PlaylistFileProvider::playlists()
{
    DEBUG_BLOCK
    return m_playlists;
}

Meta::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks )
{
    Meta::PlaylistFormat format = m_defaultFormat;
    Meta::Playlist *playlist = 0;
    switch( format )
    {
        case Meta::PLS:
            playlist = new Meta::PLSPlaylist( tracks );
            break;
        case Meta::M3U:
            playlist = new Meta::M3UPlaylist( tracks );
            break;
        case Meta::XSPF:
            playlist = new Meta::XSPFPlaylist( tracks );
            break;
        default:
            debug() << "unknown type!";
            break;
    }

    return Meta::PlaylistPtr( playlist );
}

Meta::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks, const QString &name )
{
    return Meta::PlaylistPtr();
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
        if ( !Meta::isPlaylist( ( *it ).fileName() ) )
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

    KUrl
    newPlaylistFilePath( const QString & fileExtension )
    {
        int trailingNumber = 1;
        KLocalizedString fileName = ki18n("Playlist_%1");
        KUrl url( Amarok::saveLocation( "playlists" ) );
        url.addPath( fileName.subs( trailingNumber ).toString() );

        while( QFileInfo( url.path() ).exists() )
            url.setFileName( fileName.subs( ++trailingNumber ).toString() );

        return KUrl( url.path() + fileExtension );
    }
}
