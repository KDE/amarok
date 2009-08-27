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
#include "playlist/PlaylistModelStack.h"
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
        KUrl url( key );
        QString groups = loadedPlaylistsConfig().readEntry( key );
        Meta::PlaylistFilePtr playlist = Meta::loadPlaylistFile( url );
        if( playlist.isNull() )
        {
            The::statusBar()->longMessage(
                    i18n("The playlist file \"%1\" could not be loaded!").arg( url.fileName() ),
                    StatusBar::Error
                );
            continue;
        }

        if( !groups.isEmpty() && playlist->isWritable() )
            playlist->setGroups( groups.split( ",",  QString::SkipEmptyParts ) );

        m_playlists << Meta::PlaylistPtr::dynamicCast( playlist );
    }
}

PlaylistFileProvider::~PlaylistFileProvider()
{
    DEBUG_BLOCK
    //Write loaded playlists to config file
    debug() << m_playlists.size()  << " Playlists loaded";
    foreach( Meta::PlaylistPtr playlist, m_playlists )
    {
        KUrl url = playlist->retrievableUrl();
        if( loadedPlaylistsConfig().keyList().contains( url.url() ) )
            continue;
        debug() << "storing: " << url.url();

        loadedPlaylistsConfig().writeEntry( url.url(), playlist->groups() );
    }
    loadedPlaylistsConfig().sync();
}

QString
PlaylistFileProvider::prettyName() const
{
    return i18n( "Playlist Files on Disk" );
}

Meta::PlaylistList
PlaylistFileProvider::playlists()
{
    return m_playlists;
}

QList<QAction *>
PlaylistFileProvider::playlistActions( Meta::PlaylistPtr playlist )
{
    Q_UNUSED( playlist );
    QList<QAction *> actions;

    return actions;
}

QList<QAction *>
PlaylistFileProvider::trackActions( Meta::PlaylistPtr playlist, int trackIndex )
{
    Q_UNUSED( playlist );
    Q_UNUSED( trackIndex );
    QList<QAction *> actions;

    return actions;
}


Meta::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks )
{
    return save( tracks, QDateTime::currentDateTime().toString( "ddd MMMM d yy hh:mm") );
}

Meta::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks, const QString &name )
{
    DEBUG_BLOCK
    KUrl path( Amarok::saveLocation( "playlists" ) );
    path.addPath( name );
    if( QFileInfo( path.toLocalFile() ).exists() )
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
    DEBUG_BLOCK
    foreach( const Meta::PlaylistPtr playlist, m_playlists )
    {
        Meta::PlaylistFilePtr playlistFile =
                Meta::PlaylistFilePtr::dynamicCast( playlist );
        if( !playlistFile )
        {
            error() << "Could not cast down.";
            error() << "m_playlists got corrupted! " << __FILE__ << ":" << __LINE__;
            continue;
        }
        if( playlistFile->retrievableUrl() == path )
        {
            debug() << "Playlist " << path.path() << " was already imported";
            return false;
        }
    }

    debug() << "Importing playlist file " << path;
    if( path == Playlist::ModelStack::instance()->source()->defaultPlaylistPath() )
    {
        error() << "trying to load saved session playlist at %s" << path.path();
        return false;
    }

    Meta::PlaylistFilePtr playlist = Meta::loadPlaylistFile( path );
    if( !playlist )
        return false;
    m_playlists << Meta::PlaylistPtr::dynamicCast( playlist );
    emit updated();
    return true;
}

void
PlaylistFileProvider::rename( Meta::PlaylistPtr playlist, const QString &newName )
{
    DEBUG_BLOCK
    Q_UNUSED(playlist);
    Q_UNUSED(newName);
}

void
PlaylistFileProvider::deletePlaylists( Meta::PlaylistList playlistList )
{
    DEBUG_BLOCK
    foreach( Meta::PlaylistPtr playlist, playlistList )
    {
        Meta::PlaylistFilePtr playlistFile = Meta::PlaylistFilePtr::dynamicCast( playlist );
        if( playlistFile.isNull() )
        {
            error() << "Could not cast to playlistFilePtr at " << __FILE__ << ":" << __LINE__;
            continue;
        }
        m_playlists.removeAll( playlist );
        loadedPlaylistsConfig().deleteEntry( playlistFile->retrievableUrl().url() );
        QFile::remove( playlistFile->retrievableUrl().path() );
    }
    loadedPlaylistsConfig().sync();
    emit updated();
}

KConfigGroup
PlaylistFileProvider::loadedPlaylistsConfig()
{
    return Amarok::config( "Loaded Playlist Files" );
}
