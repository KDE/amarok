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

#include "PlaylistFileProvider.h"
#include "App.h"
#include "amarokconfig.h"
#include "playlists/impl/file/PlaylistFileSupport.h"
#include "EditablePlaylistCapability.h"
#include "Amarok.h"
#include "Debug.h"
#include "playlists/impl/file/m3u/M3UPlaylist.h"
#include "playlists/impl/file/pls/PLSPlaylist.h"
#include "playlists/impl/file/xspf/XSPFPlaylist.h"
#include "playlist/PlaylistModelStack.h"
#include "StatusBar.h"

#include <KDialog>
#include <KInputDialog>
#include <KLocale>
#include <KUrl>

#include <QLabel>
#include <QString>

//For removing multiple tracks from different playlists with one QAction
typedef QMultiMap<Meta::PlaylistPtr, Meta::TrackPtr> PlaylistTrackMap;
Q_DECLARE_METATYPE( PlaylistTrackMap )

PlaylistFileProvider::PlaylistFileProvider()
 : UserPlaylistProvider()
 , m_playlistsLoaded( false )
 , m_defaultFormat( Meta::XSPF )
 , m_removeTrackAction( 0 )
{
    //playlists are lazy loaded
}

PlaylistFileProvider::~PlaylistFileProvider()
{
    DEBUG_BLOCK
    //remove all, well add them again soon
    loadedPlaylistsConfig().deleteGroup();
    //Write loaded playlists to config file
    debug() << m_playlists.size()  << " Playlists loaded";
    foreach( Meta::PlaylistPtr playlist, m_playlists )
    {
        KUrl url = playlist->retrievableUrl();
        //only save files NOT in "playlists", those are automatically loaded.
        if( url.upUrl().equals( Amarok::saveLocation( "playlists" ) ) )
            continue;

        //debug() << "storing to rc-file: " << url.url();

        loadedPlaylistsConfig().writeEntry( url.url(), playlist->groups() );
    }
    loadedPlaylistsConfig().sync();
}

QString
PlaylistFileProvider::prettyName() const
{
    return i18n( "Playlist Files on Disk" );
}

int
PlaylistFileProvider::playlistCount() const
{
    if( m_playlistsLoaded )
        return m_playlists.count();
    //count the entries in the config file
    return loadedPlaylistsConfig().keyList().count();
}

Meta::PlaylistList
PlaylistFileProvider::playlists()
{
    if( !m_playlistsLoaded )
        loadPlaylists();
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
    Q_UNUSED( trackIndex );
    QList<QAction *> actions;
    //no actions if this is not one of ours
    if( !m_playlists.contains( playlist ) )
        return actions;

    if( trackIndex < 0 )
        return actions;

    int trackCount = playlist->trackCount();
    if( trackCount == -1 )
        trackCount = playlist->tracks().size();

    if( trackIndex >= trackCount )
        return actions;

    if( m_removeTrackAction == 0 )
    {
        m_removeTrackAction = new QAction(
                    KIcon( "media-track-remove-amarok" ),
                    i18nc( "Remove a track from a saved playlist", "Remove From \"%1\"" )
                        .arg( playlist->name() ),
                    this
                );
        m_removeTrackAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_removeTrackAction, SIGNAL( triggered() ), SLOT( slotRemove() ) );
    }
    else
    {
        m_removeTrackAction->setText( i18nc( "Remove a track from a saved playlist",
                                             "Remove From \"%1\"" ).arg( playlist->name() ) );
    }

    //Add the playlist/track combination to a QMultiMap that is stored in the action.
    //In the slot we use this data to remove that track from the playlist.
    PlaylistTrackMap playlistMap = m_removeTrackAction->data().value<PlaylistTrackMap>();
    Meta::TrackPtr track = playlist->tracks()[trackIndex];
    //only add action to map if playlist/track combo is not in there yet.
    if( !playlistMap.keys().contains( playlist ) ||
           !playlistMap.values( playlist ).contains( track )
      )
    {
        playlistMap.insert( playlist, track );
    }
    m_removeTrackAction->setData( QVariant::fromValue( playlistMap ) );

    if( playlistMap.keys().count() > 1 )
        m_removeTrackAction->setText( i18n( "Remove" ) );

    actions << m_removeTrackAction;

    return actions;
}


Meta::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks )
{
    return save( tracks, QDateTime::currentDateTime().toString( "ddd MMMM d yy hh:mm") + ".xspf" );
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
    if( !name.isNull() && !ext.isEmpty() )
        format = Meta::getFormat( path );

    Meta::PlaylistFile *playlistFile = 0;
    switch( format )
    {
        case Meta::PLS:
            playlistFile = new Meta::PLSPlaylist( tracks );
            break;
        case Meta::M3U:
            playlistFile = new Meta::M3UPlaylist( tracks );
            break;
        case Meta::XSPF:
            playlistFile = new Meta::XSPFPlaylist( tracks );
            break;
        default:
            debug() << QString("Do not support filetype with extension \"%1!\"").arg( ext );
            return Meta::PlaylistPtr();
    }
    playlistFile->setName( name );
    debug() << "Forcing save of playlist!";
    playlistFile->save( path, true );

    Meta::PlaylistPtr playlistPtr( playlistFile );
    m_playlists << playlistPtr;
    //just in case there wasn't one loaded before.
    m_playlistsLoaded = true;
    emit updated();

    return playlistPtr;
}

bool
PlaylistFileProvider::import( const KUrl &path )
{
    DEBUG_BLOCK
    if( !path.isValid() )
    {
        error() << "path is not valid!";
        return false;
    }

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
    //just in case there wasn't one loaded before.
    m_playlistsLoaded = true;
    emit updated();
    return true;
}

void
PlaylistFileProvider::rename( Meta::PlaylistPtr playlist, const QString &newName )
{
    DEBUG_BLOCK
    playlist->setName( newName );
}

void
PlaylistFileProvider::deletePlaylists( Meta::PlaylistList playlistList )
{
    DEBUG_BLOCK
    KDialog dialog( The::mainWindow() );
    dialog.setCaption( i18n( "Confirm Delete" ) );
    dialog.setButtons( KDialog::Ok | KDialog::Cancel );
    QLabel label( i18np( "Are you sure you want to delete this playlist?",
                         "Are you sure you want to delete these %1 playlist files?",
                         playlistList.count() )
                    , &dialog
                  );
    dialog.setButtonText( KDialog::Ok, i18n( "Yes, delete from disk." ) );
    dialog.setMainWidget( &label );
    if( dialog.exec() != QDialog::Accepted )
        return;

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

void
PlaylistFileProvider::loadPlaylists()
{
    DEBUG_BLOCK
    //load the playlists defined in the config
    QStringList keys = loadedPlaylistsConfig().keyList();
    debug() << "keys " << keys;

    //ConfigEntry: name, file
    foreach( const QString &key, keys )
    {
        KUrl url( key );
        //Don't load these from the config file, they are read from the directory anyway
        if( url.upUrl().equals( Amarok::saveLocation( "playlists" ) ) )
            continue;

        QString groups = loadedPlaylistsConfig().readEntry( key );
        Meta::PlaylistFilePtr playlist = Meta::loadPlaylistFile( url );
        if( playlist.isNull() )
        {
            The::statusBar()->longMessage(
                    i18n("The playlist file \"%1\" could not be loaded.").arg( url.fileName() ),
                    StatusBar::Error
                );
            continue;
        }
        playlist->setProvider( this );

        if( !groups.isEmpty() && playlist->isWritable() )
            playlist->setGroups( groups.split( ',',  QString::SkipEmptyParts ) );

        m_playlists << Meta::PlaylistPtr::dynamicCast( playlist );
    }

    //also add all files in the $KDEHOME/share/apps/amarok/playlists
    QDir playlistDir = QDir( Amarok::saveLocation( "playlists" ), "",
                             QDir::Name,
                             QDir::Files | QDir::Readable );
    foreach( const QString &file, playlistDir.entryList() )
    {
        KUrl url( playlistDir.path() );
        url.addPath( file );
        debug() << QString( "Trying to open %1 as a playlist file" ).arg( url.url() );
        Meta::PlaylistFilePtr playlist = Meta::loadPlaylistFile( url );
        if( playlist.isNull() )
        {
            The::statusBar()->longMessage(
                    i18n("The playlist file \"%1\" could not be loaded.").arg( url.fileName() ),
                    StatusBar::Error
                );
            continue;
        }
        playlist->setProvider( this );

        m_playlists << Meta::PlaylistPtr::dynamicCast( playlist );
    }

    m_playlistsLoaded = true;
}

void
PlaylistFileProvider::slotRemove()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    PlaylistTrackMap playlistMap = action->data().value<PlaylistTrackMap>();
    QList< Meta::PlaylistPtr > uniquePlaylists = playlistMap.uniqueKeys();

    foreach( Meta::PlaylistPtr playlist, uniquePlaylists )
    {
        QList< Meta::TrackPtr > tracks = playlistMap.values( playlist );
        foreach( Meta::TrackPtr track, tracks )
            playlist->removeTrack( playlist->tracks().indexOf( track ) );
    }

    //clear the data
    action->setData( QVariant() );
}

KConfigGroup
PlaylistFileProvider::loadedPlaylistsConfig() const
{
    return Amarok::config( "Loaded Playlist Files" );
}
