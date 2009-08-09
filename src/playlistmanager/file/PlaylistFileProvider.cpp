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

#include <KInputDialog>
#include <KLocale>
#include <KUrl>

PlaylistFileProvider::PlaylistFileProvider()
 : UserPlaylistProvider()
 , m_defaultFormat( Meta::XSPF )
{
    DEBUG_BLOCK
    //load the playlists defined in the config
    QStringList keys = loadedPlaylistsConfig().keyList();
    debug() << "keys " << keys;

    //ConfigEntry: name, file
    foreach( const QString &key, keys )
    {
        QStringList configEntry = loadedPlaylistsConfig().readXdgListEntry( key );
        Meta::PlaylistFilePtr playlist =
                Meta::loadPlaylistFile( KUrl( configEntry[1] ).path() );
        if( playlist->isWritable() )
        {
            QString name = configEntry[0];
            playlist->setName( name );
            if( configEntry.length() >= 3 )
                playlist->setGroups( configEntry[2].split( ",",  QString::SkipEmptyParts ) );
        }
        m_playlists << Meta::PlaylistPtr::dynamicCast( playlist );
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
        configEntry << playlist->groups();

        loadedPlaylistsConfig().writeXdgListEntry(
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
    DEBUG_BLOCK
    return save( tracks, QDateTime::currentDateTime().toString( "ddd MMMM d yy hh:mm") );
}

Meta::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks, const QString &name )
{
    KUrl path( Amarok::saveLocation( "playlists" ) );
    path.addPath( name );
    if( QFileInfo( path.path() ).exists() )
    {
        //TODO:request overwrite
        return Meta::PlaylistPtr();
    }
    QString ext = Amarok::extension( path.fileName() );
    Meta::PlaylistFormat format = m_defaultFormat;
    if( !ext.isEmpty() )
        format = Meta::getFormat( path );

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
    Meta::PlaylistPtr playlistPtr( playlist );
    m_playlists << playlistPtr;
    emit updated();

    return playlistPtr;
}

bool
PlaylistFileProvider::import( const KUrl &path )
{
    Meta::PlaylistFilePtr playlist = Meta::loadPlaylistFile( path );
    if( !playlist )
        return false;
    m_playlists << Meta::PlaylistPtr::dynamicCast( playlist );
    emit updated();
    return true;
}

KConfigGroup
PlaylistFileProvider::loadedPlaylistsConfig()
{
    return Amarok::config( "Loaded Playlist Files" );
}
