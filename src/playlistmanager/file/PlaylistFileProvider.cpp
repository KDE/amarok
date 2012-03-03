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
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core/capabilities/EditablePlaylistCapability.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "core-impl/playlists/types/file/m3u/M3UPlaylist.h"
#include "core-impl/playlists/types/file/pls/PLSPlaylist.h"
#include "core-impl/playlists/types/file/xspf/XSPFPlaylist.h"
#include "playlist/PlaylistModelStack.h"
#include "playlistmanager/PlaylistManager.h"

#include <KDialog>
#include <KInputDialog>
#include <KLocale>
#include <KUrl>

#include <QAction>
#include <QDir>
#include <QLabel>
#include <QString>
#include <QTimer>

//For removing multiple tracks from different playlists with one QAction
typedef QMultiMap<Playlists::PlaylistPtr, Meta::TrackPtr> PlaylistTrackMap;
Q_DECLARE_METATYPE( PlaylistTrackMap )

using Playlist::ModelStack;

namespace Playlists {

PlaylistFileProvider::PlaylistFileProvider()
 : UserPlaylistProvider()
 , m_playlistsLoaded( false )
 , m_renameAction( 0 )
 , m_deleteAction( 0 )
 , m_removeTrackAction( 0 )
 , m_saveLaterTimer( 0 )
{
    //playlists are lazy loaded but we can count how many we'll load already
    QStringList keys = loadedPlaylistsConfig().keyList();
    foreach( const QString &key, keys )
    {
        KUrl url( key );
        //Don't load these from the config file, they are read from the directory anyway
        if( url.upUrl().equals( Amarok::saveLocation( "playlists" ) ) )
            continue;
        m_urlsToLoad << url;
    }
    //also add all files in the $KDEHOME/share/apps/amarok/playlists
    QDir playlistDir = QDir( Amarok::saveLocation( "playlists" ), "",
                             QDir::Name,
                             QDir::Files | QDir::Readable );
    foreach( const QString &file, playlistDir.entryList() )
    {
        KUrl url( playlistDir.path() );
        url.addPath( file );
        if( Playlists::isPlaylist( url ) )
            m_urlsToLoad << url;
    }
}

PlaylistFileProvider::~PlaylistFileProvider()
{
    DEBUG_BLOCK
    //remove all, well add them again soon
    loadedPlaylistsConfig().deleteGroup();
    //Write loaded playlists to config file
    debug() << m_playlists.size()  << " Playlists loaded";
    foreach( Playlists::PlaylistFilePtr playlistFile, m_playlists )
    {
        KUrl url = playlistFile->uidUrl();
        //only save files NOT in "playlists", those are automatically loaded.
        if( url.upUrl().equals( Amarok::saveLocation( "playlists" ) ) )
            continue;

        //debug() << "storing to rc-file: " << url.url();

        loadedPlaylistsConfig().writeEntry( url.url(), playlistFile->groups() );
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
    return m_playlists.count() + m_urlsToLoad.count();
}

Playlists::PlaylistList
PlaylistFileProvider::playlists()
{
    Playlists::PlaylistList playlists;

    if( !m_playlistsLoaded )
    {
        //trigger a lazy load the playlists
        QTimer::singleShot(0, this, SLOT(loadPlaylists()) );
        return playlists;
    }

    foreach( const Playlists::PlaylistFilePtr &playlistFile, m_playlists )
    {
        Playlists::PlaylistPtr playlist = Playlists::PlaylistPtr::dynamicCast( playlistFile );
        if( !playlist.isNull() )
            playlists << playlist;
    }
    return playlists;
}

QList<QAction *>
PlaylistFileProvider::playlistActions( Playlists::PlaylistPtr playlist )
{
    QList<QAction *> actions;

    Playlists::PlaylistFilePtr playlistFile = Playlists::PlaylistFilePtr::dynamicCast( playlist );
    if( !playlistFile )
    {
        error() << "Action requested for a non-file playlist";
        return actions;
    }

    if( m_renameAction == 0 )
    {
        m_renameAction =  new QAction( KIcon( "media-track-edit-amarok" ), i18n( "&Rename..." ), this );
        m_renameAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_renameAction, SIGNAL( triggered() ), this, SLOT( slotRename() ) );
    }
    //only one playlist can be renamed at a time.
    if( m_renameAction->data().isNull() )
        m_renameAction->setData( QVariant::fromValue( playlistFile ) );

    actions << m_renameAction;

    if( m_deleteAction == 0 )
    {
        m_deleteAction = new QAction( KIcon( "media-track-remove-amarok" ), i18n( "&Delete..." ),
                                      this );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_deleteAction, SIGNAL( triggered() ), SLOT( slotDelete() ) );
    }
    m_deleteAction->setObjectName( "deleteAction" );

    Playlists::PlaylistFileList actionList =
            m_deleteAction->data().value<Playlists::PlaylistFileList>();
    actionList << playlistFile;
    m_deleteAction->setData( QVariant::fromValue( actionList ) );

    actions << m_deleteAction;

    return actions;
}

QList<QAction *>
PlaylistFileProvider::trackActions( Playlists::PlaylistPtr playlist, int trackIndex )
{
    Q_UNUSED( trackIndex );
    QList<QAction *> actions;

    if( trackIndex < 0 )
        return actions;

    int trackCount = playlist->trackCount();
    if( trackCount == -1 )
        trackCount = playlist->tracks().size();

    if( trackIndex >= trackCount )
        return actions;

    if( m_removeTrackAction == 0 )
    {
        m_removeTrackAction = new QAction( this );
        m_removeTrackAction->setIcon( KIcon( "media-track-remove-amarok" ) );
        m_removeTrackAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_removeTrackAction, SIGNAL( triggered() ), SLOT( slotRemove() ) );
    }

    m_removeTrackAction->setObjectName( "deleteAction" );
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
        m_removeTrackAction->setText( i18n( "Remove tracks" ) );
    else
        m_removeTrackAction->setText( i18nc( "Remove a track from a saved playlist",
                                             "Remove From \"%1\"", playlist->name() ) );

    if( playlistMap.keys().count() > 1 )
        m_removeTrackAction->setText( i18n( "Remove" ) );

    actions << m_removeTrackAction;

    return actions;
}

Playlists::PlaylistPtr
PlaylistFileProvider::save( const Meta::TrackList &tracks, const QString &name )
{
    DEBUG_BLOCK
    QString filename = name.isEmpty() ? QDateTime::currentDateTime().toString( "ddd MMMM d yy hh-mm") : name;
    filename.replace( QLatin1Char('/'), QLatin1Char('-') );

    QString ext = Amarok::extension( filename );
    Playlists::PlaylistFormat format = Playlists::getFormat( ext );
    if( format == Unknown )
    {
        format = Playlists::XSPF;
        filename.append( QLatin1String( ".xspf" ) );
    }

    KUrl path( Amarok::saveLocation( "playlists" ) );
    path.addPath( Amarok::vfatPath( filename ) );
    if( QFileInfo( path.toLocalFile() ).exists() )
    {
        //TODO:request overwrite
        return Playlists::PlaylistPtr();
    }

    Playlists::PlaylistFile *playlistFile = 0;
    switch( format )
    {
        case Playlists::PLS:
            playlistFile = new Playlists::PLSPlaylist( tracks );
            break;
        case Playlists::M3U:
            playlistFile = new Playlists::M3UPlaylist( tracks );
            break;
        case Playlists::XSPF:
            playlistFile = new Playlists::XSPFPlaylist( tracks );
            break;
        default:
            error() << QString("Do not support filetype with extension \"%1!\"").arg( ext );
            return Playlists::PlaylistPtr();
    }
    playlistFile->setName( filename );
    playlistFile->save( path, true );
    playlistFile->setProvider( this );

    Playlists::PlaylistFilePtr playlistPtr( playlistFile );
    m_playlists << playlistPtr;
    //just in case there wasn't one loaded before.
    m_playlistsLoaded = true;
    Playlists::PlaylistPtr playlist = Playlists::PlaylistPtr::dynamicCast( playlistPtr );
    emit playlistAdded( playlist );

    return playlist;
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

    foreach( Playlists::PlaylistFilePtr playlistFile, m_playlists )
    {
        if( !playlistFile )
        {
            error() << "Could not cast down.";
            error() << "m_playlists got corrupted! " << __FILE__ << ":" << __LINE__;
            continue;
        }
        if( playlistFile->uidUrl() == path )
        {
            debug() << "Playlist " << path.path() << " was already imported";
            return false;
        }
    }

    debug() << "Importing playlist file " << path;
    if( path == Amarok::defaultPlaylistPath() )
    {
        error() << "trying to load saved session playlist at %s" << path.path();
        return false;
    }

    Playlists::PlaylistFilePtr playlistFile = Playlists::loadPlaylistFile( path );
    if( !playlistFile )
        return false;
    playlistFile->setProvider( this );

    m_playlists << playlistFile;
    //just in case there wasn't one loaded before.
    m_playlistsLoaded = true;
    emit playlistAdded( Playlists::PlaylistPtr::dynamicCast( playlistFile ) );
    return true;
}

void
PlaylistFileProvider::rename( Playlists::PlaylistPtr playlist, const QString &newName )
{
    DEBUG_BLOCK
    playlist->setName( newName );
}

bool
PlaylistFileProvider::deletePlaylists( Playlists::PlaylistList playlists )
{
    Playlists::PlaylistFileList playlistFiles;
    foreach( Playlists::PlaylistPtr playlist, playlists )
    {
        Playlists::PlaylistFilePtr playlistFile =
                Playlists::PlaylistFilePtr::dynamicCast( playlist );
        if( !playlistFile.isNull() )
            playlistFiles << playlistFile;
    }
    return deletePlaylistFiles( playlistFiles );
}

bool
PlaylistFileProvider::deletePlaylistFiles( Playlists::PlaylistFileList playlistFiles )
{
    foreach( Playlists::PlaylistFilePtr playlistFile, playlistFiles )
    {
        m_playlists.removeAll( playlistFile );
        loadedPlaylistsConfig().deleteEntry( playlistFile->uidUrl().url() );
        QFile::remove( playlistFile->uidUrl().path() );
        emit playlistRemoved( Playlists::PlaylistPtr::dynamicCast( playlistFile ) );
    }
    loadedPlaylistsConfig().sync();

    return true;
}

void
PlaylistFileProvider::loadPlaylists()
{
    if( m_urlsToLoad.isEmpty() )
        return;

    //arbitrary number of playlists to load during one mainloop run: 5
    for( int i = 0; i < qMin( m_urlsToLoad.count(), 5 ); i++ )
    {
        KUrl url = m_urlsToLoad.takeFirst();
        QString groups = loadedPlaylistsConfig().readEntry( url.url() );
        Playlists::PlaylistFilePtr playlist = Playlists::loadPlaylistFile( url );
        if( playlist.isNull() )
        {
            Amarok::Components::logger()->longMessage(
                    i18n("The playlist file \"%1\" could not be loaded.", url.fileName() ),
                    Amarok::Logger::Error
                );
            continue;
        }

        //since class PlaylistFile can be used without a collection we set it manually
        playlist->setProvider( this );

        if( !groups.isEmpty() && playlist->isWritable() )
            playlist->setGroups( groups.split( ',',  QString::SkipEmptyParts ) );

        m_playlists << playlist;
        emit playlistAdded( Playlists::PlaylistPtr::dynamicCast( playlist ) );
    }

    //give the mainloop time to run
    if( !m_urlsToLoad.isEmpty() )
        QTimer::singleShot( 0, this, SLOT(loadPlaylists()) );
}

void
PlaylistFileProvider::saveLater( Playlists::PlaylistFilePtr playlist )
{
    //WARNING: this assumes the playlistfile uses it's m_url for uidUrl
    if( playlist->uidUrl().isEmpty() )
        return;

    if( !m_saveLaterPlaylists.contains( playlist ) )
        m_saveLaterPlaylists << playlist;

    if( !m_saveLaterTimer )
    {
        m_saveLaterTimer = new QTimer( this );
        m_saveLaterTimer->setSingleShot( true );
        m_saveLaterTimer->setInterval( 0 );
        connect( m_saveLaterTimer, SIGNAL(timeout()), SLOT(slotSaveLater()) );
    }

    m_saveLaterTimer->start();
}

void
PlaylistFileProvider::slotSaveLater() //SLOT
{
    foreach( Playlists::PlaylistFilePtr playlist, m_saveLaterPlaylists )
    {
        KUrl url = playlist->uidUrl();
        playlist->save( url, true ); //TODO: read relative type when loading
    }

    m_saveLaterPlaylists.clear();
}

void
PlaylistFileProvider::slotDelete()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    //only one playlist can be selected at this point
    Playlists::PlaylistFileList playlists = action->data().value<Playlists::PlaylistFileList>();

    if( playlists.count() == 0 )
        return;

    KDialog dialog;
    dialog.setCaption( i18n( "Confirm Delete" ) );
    dialog.setButtons( KDialog::Ok | KDialog::Cancel );
    QLabel label( i18np( "Are you sure you want to delete this playlist?",
                         "Are you sure you want to delete these %1 playlist files?",
                         playlists.count() )
                    , &dialog
                  );
    //TODO:include a text area with all the names of the playlists
    dialog.setButtonText( KDialog::Ok, i18n( "Yes, delete from disk." ) );
    dialog.setMainWidget( &label );
    if( dialog.exec() == QDialog::Accepted )
        deletePlaylistFiles( playlists );
}

void
PlaylistFileProvider::slotRename()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    //only one playlist can be renamed at a time.
    Playlists::PlaylistFilePtr playlist = action->data().value<Playlists::PlaylistFilePtr>();
    if( playlist.isNull() )
        return;

    //TODO: inline rename
    bool ok;
    const QString newName = KInputDialog::getText( i18n("Change playlist"),
                i18n("Enter new name for playlist:"), playlist->name(),
                                                   &ok );
    if( ok )
        playlist->setName( newName.trimmed() );
}

void
PlaylistFileProvider::slotRemove()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    PlaylistTrackMap playlistMap = action->data().value<PlaylistTrackMap>();
    QList< Playlists::PlaylistPtr > uniquePlaylists = playlistMap.uniqueKeys();

    foreach( Playlists::PlaylistPtr playlist, uniquePlaylists )
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

} //namespace Playlists

