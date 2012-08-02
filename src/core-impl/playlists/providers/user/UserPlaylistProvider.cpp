/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"

#include "playlistmanager/PlaylistManager.h"
#include "amarokconfig.h"

#include <KDialog>
#include <KFileDialog>
#include <kabstractfilewidget.h> // KFileDialog needs this. It's a scandal.
#include <KLocalizedString>

#include <QAction>
#include <QCheckBox>
#include <QLabel>

// For removing multiple tracks from different playlists with one QAction
typedef QMultiMap<Playlists::PlaylistPtr, int> PlaylistTrackMap;
Q_DECLARE_METATYPE( PlaylistTrackMap )

Playlists::UserPlaylistProvider::UserPlaylistProvider( QObject *parent )
    : PlaylistProvider( parent )
{
    m_deleteAction = new QAction( KIcon( "media-track-remove-amarok" ), i18n( "&Delete..." ), this );
    m_deleteAction->setProperty( "popupdropper_svg_id", "delete" );
    // object name must match one in PlaylistBrowserNS::PlaylistBrowserView
    m_deleteAction->setObjectName( "deleteAction" );
    // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
    m_deleteAction->setShortcut( Qt::Key_Delete );
    connect( m_deleteAction, SIGNAL(triggered()), SLOT(slotDelete()) );

    m_renameAction =  new QAction( KIcon( "media-track-edit-amarok" ), i18n( "&Rename..." ), this );
    m_renameAction->setProperty( "popupdropper_svg_id", "edit" );
    // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
    m_renameAction->setShortcut( Qt::Key_F2 );
    connect( m_renameAction, SIGNAL(triggered()), this, SLOT(slotRename()) );

    m_exportAction = new QAction( KIcon( "document-export-amarok" ), i18n("&Export Playlist As..."), this );
    connect( m_exportAction, SIGNAL(triggered()), this, SLOT(slotExport()) );

    m_removeTrackAction = new QAction( KIcon( "media-track-remove-amarok" ), QString( "Placeholder" ), this );
    m_removeTrackAction->setProperty( "popupdropper_svg_id", "delete" );
    // object name must match one in PlaylistBrowserNS::PlaylistBrowserView
    m_removeTrackAction->setObjectName( "deleteAction" );
    // key shortcut is only for display purposes here, actual one is determined by View in Model/View classes
    m_removeTrackAction->setShortcut( Qt::Key_Delete );
    connect( m_removeTrackAction, SIGNAL(triggered()), SLOT(slotRemoveTrack()) );
}

Playlists::UserPlaylistProvider::~UserPlaylistProvider()
{
}

int
Playlists::UserPlaylistProvider::category() const
{
     return Playlists::UserPlaylist;
}

QList<QAction *>
Playlists::UserPlaylistProvider::playlistActions( Playlists::PlaylistPtr playlist )
{
    QList<QAction *> actions;

    Playlists::PlaylistList actionList = m_deleteAction->data().value<Playlists::PlaylistList>();
    actionList << playlist;
    m_deleteAction->setData( QVariant::fromValue( actionList ) );
    actions << m_deleteAction;

    m_renameAction->setData( QVariant::fromValue( playlist ) );
    actions << m_renameAction;

    m_exportAction->setData( QVariant::fromValue( playlist ) );
    actions << m_exportAction;

    return actions;
}

QList<QAction *>
Playlists::UserPlaylistProvider::trackActions( Playlists::PlaylistPtr playlist, int trackIndex )
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

    // Add the playlist/track combination to a QMultiMap that is stored in the action.
    // In the slot we use this data to remove that track from the playlist.
    PlaylistTrackMap playlistMap = m_removeTrackAction->data().value<PlaylistTrackMap>();

    //only add action to map if playlist/track combo is not in there yet.
    if( !playlistMap.keys().contains( playlist ) ||
        !playlistMap.values( playlist ).contains( trackIndex ) )
    {
        playlistMap.insert( playlist, trackIndex );
    }
    m_removeTrackAction->setData( QVariant::fromValue( playlistMap ) );

    const int actionTrackCount = playlistMap.count();
    const int playlistCount = playlistMap.uniqueKeys().count();
    if( playlistCount > 1 )
    {
        m_removeTrackAction->setText( i18ncp( "Number of playlists is >= 2",
            "Remove a track from %2 playlists", "Remove %1 tracks from %2 playlists",
            actionTrackCount, playlistCount ) );
    }
    else
    {
        m_removeTrackAction->setText( i18ncp( "%2 is saved playlist name",
            "Remove a track from %2", "Remove %1 tracks from %2", actionTrackCount,
            playlist->name() ) );
    }
    actions << m_removeTrackAction;

    return actions;
}

void
Playlists::UserPlaylistProvider::slotDelete()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Playlists::PlaylistList playlists = action->data().value<Playlists::PlaylistList>();
    if( playlists.count() == 0 )
        return;

    KDialog dialog;
    dialog.setCaption( i18n( "Confirm Delete" ) );
    dialog.setButtons( KDialog::Ok | KDialog::Cancel );
    QLabel label( i18np( "Are you sure you want to delete this playlist?",
                         "Are you sure you want to delete these %1 playlists?",
                         playlists.count() )
                    , &dialog
                  );
    //TODO:include a text area with all the names of the playlists
    dialog.setButtonText( KDialog::Ok, i18nc( "%1 is playlist provider pretty name",
                                              "Yes, delete from %1.", prettyName() ) );
    dialog.setMainWidget( &label );
    if( dialog.exec() == QDialog::Accepted )
        deletePlaylists( playlists );
}

void
Playlists::UserPlaylistProvider::slotRename()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Playlists::PlaylistPtr playlist = action->data().value<Playlists::PlaylistPtr>();
    if( playlist.isNull() )
        return;

    The::playlistManager()->rename( playlist );  // initiates inline rename of the playlist
}

void
Playlists::UserPlaylistProvider::slotExport()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Playlists::PlaylistPtr playlist = action->data().value<Playlists::PlaylistPtr>();
    if( playlist.isNull() )
        return;

    // --- display save location dialog
    // compare with MainWindow::exportPlaylist
    // TODO: have this code only once
    KFileDialog fileDialog( KUrl("kfiledialog:///amarok-playlist-export"), QString(), 0 );
    QCheckBox *saveRelativeCheck = new QCheckBox( i18n("Use relative path for &saving") );
    saveRelativeCheck->setChecked( AmarokConfig::relativePlaylist() );

    QStringList supportedMimeTypes;
    supportedMimeTypes << "audio/x-mpegurl"; //M3U
    supportedMimeTypes << "audio/x-scpls"; //PLS
    supportedMimeTypes << "application/xspf+xml"; //XSPF

    fileDialog.setMimeFilter( supportedMimeTypes, supportedMimeTypes.first() );
    fileDialog.fileWidget()->setCustomWidget( saveRelativeCheck );
    fileDialog.setOperationMode( KFileDialog::Saving );
    fileDialog.setMode( KFile::File );
    fileDialog.setCaption( i18n("Save As") );
    fileDialog.setObjectName( "PlaylistExport" );

    fileDialog.exec();

    QString playlistPath = fileDialog.selectedFile();

    // --- actually save the playlist
    if( !playlistPath.isEmpty() )
        exportPlaylistFile( playlist->tracks(), playlistPath, saveRelativeCheck->isChecked() );
}

void
Playlists::UserPlaylistProvider::slotRemoveTrack()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    PlaylistTrackMap playlistMap = action->data().value<PlaylistTrackMap>();
    foreach( Playlists::PlaylistPtr playlist, playlistMap.uniqueKeys() )
    {
        QList<int> trackIndices = playlistMap.values( playlist );
        qSort( trackIndices );
        int removed = 0;
        foreach( int trackIndex, trackIndices )
        {
            playlist->removeTrack( trackIndex - removed /* account for already removed */ );
            removed++;
        }
    }

    //clear the data
    action->setData( QVariant() );
}

#include "UserPlaylistProvider.moc"
