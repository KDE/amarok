/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "SqlUserPlaylistProvider.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "core/support/Debug.h"
#include "MainWindow.h"
#include "core-implementations/playlists/file/m3u/M3UPlaylist.h"
#include "core-implementations/playlists/file/pls/PLSPlaylist.h"
#include "core-implementations/playlists/file/xspf/XSPFPlaylist.h"
#include "core-implementations/playlists/file/PlaylistFileSupport.h"
#include "SqlStorage.h"
#include "SvgHandler.h"
#include "UserPlaylistModel.h"

#include <KDialog>
#include <KIcon>
#include <KInputDialog>
#include <KUrl>

#include <QAction>
#include <QLabel>
#include <QMap>

static const int USERPLAYLIST_DB_VERSION = 2;
static const QString key("AMAROK_USERPLAYLIST");

typedef QMultiMap<Playlists::PlaylistPtr, Meta::TrackPtr> PlaylistTrackMap;
Q_DECLARE_METATYPE( PlaylistTrackMap )

namespace Playlists {

SqlUserPlaylistProvider::SqlUserPlaylistProvider( bool debug )
    : UserPlaylistProvider()
    , m_renameAction( 0 )
    , m_deleteAction( 0 )
    , m_removeTrackAction( 0 )
    , m_debug( debug )
{
    checkTables();
    m_root = Playlists::SqlPlaylistGroupPtr( new Playlists::SqlPlaylistGroup( QString(),
            Playlists::SqlPlaylistGroupPtr(), this ) );
}

SqlUserPlaylistProvider::~SqlUserPlaylistProvider()
{
}

Playlists::PlaylistList
SqlUserPlaylistProvider::playlists()
{
    Playlists::PlaylistList playlists;
    foreach( Playlists::SqlPlaylistPtr sqlPlaylist, m_root->allChildPlaylists() )
    {
        playlists << Playlists::PlaylistPtr::staticCast( sqlPlaylist );
    }
    return playlists;
}

void
SqlUserPlaylistProvider::rename( Playlists::PlaylistPtr playlist, const QString &newName )
{
    if( !m_debug )
    {
        KDialog dialog;
        dialog.setCaption( i18n( "Confirm Rename" ) );
        dialog.setButtons( KDialog::Ok | KDialog::Cancel );
        QLabel label( i18n( "Are you sure you want to rename this playlist to '%1'?", newName ), &dialog );
        dialog.setButtonText( KDialog::Ok, i18n( "Yes, rename this playlist." ) );
        dialog.setMainWidget( &label );
        if( dialog.exec() != QDialog::Accepted )
            return;
    }
    playlist->setName( newName.trimmed() );
}

void
SqlUserPlaylistProvider::slotDelete()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    //only one playlist can be selected at this point
    Playlists::SqlPlaylistList playlists = action->data().value<Playlists::SqlPlaylistList>();

    if( playlists.count() > 0 )
        deleteSqlPlaylists( playlists );
}

void
SqlUserPlaylistProvider::slotRename()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    //only one playlist can be renamed at a time.
    Playlists::SqlPlaylistPtr playlist = action->data().value<Playlists::SqlPlaylistPtr>();
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
SqlUserPlaylistProvider::slotRemove()
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

QList<QAction *>
SqlUserPlaylistProvider::playlistActions( Playlists::PlaylistPtr playlist )
{
    QList<QAction *> actions;

    Playlists::SqlPlaylistPtr sqlPlaylist = Playlists::SqlPlaylistPtr::dynamicCast( playlist );
    if( !sqlPlaylist )
    {
        error() << "Action requested for a non-SQL playlist";
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
        m_renameAction->setData( QVariant::fromValue( sqlPlaylist ) );

    actions << m_renameAction;

    if( m_deleteAction == 0 )
    {
        m_deleteAction = new QAction( KIcon( "media-track-remove-amarok" ), i18n( "&Delete..." ), this );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_deleteAction, SIGNAL( triggered() ), SLOT( slotDelete() ) );
    }

    Playlists::SqlPlaylistList actionList = m_deleteAction->data().value<Playlists::SqlPlaylistList>();
    actionList << sqlPlaylist;
    m_deleteAction->setData( QVariant::fromValue( actionList ) );

    actions << m_deleteAction;

    return actions;
}

QList<QAction *>
SqlUserPlaylistProvider::trackActions( Playlists::PlaylistPtr playlist, int trackIndex )
{
    Q_UNUSED( trackIndex );
    QList<QAction *> actions;

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

    actions << m_removeTrackAction;

    return actions;
}

void
SqlUserPlaylistProvider::deletePlaylists( Playlists::PlaylistList playlistList )
{
    Playlists::SqlPlaylistList sqlPlaylists;
    foreach( Playlists::PlaylistPtr playlist, playlistList )
        sqlPlaylists << Playlists::SqlPlaylistPtr::dynamicCast( playlist );
    deleteSqlPlaylists( sqlPlaylists );
}

void
SqlUserPlaylistProvider::deleteSqlPlaylists( Playlists::SqlPlaylistList playlistList )
{
    if( !m_debug )
    {
        KDialog dialog;
        dialog.setCaption( i18n( "Confirm Delete" ) );
        dialog.setButtons( KDialog::Ok | KDialog::Cancel );
        QLabel label( i18np( "Are you sure you want to delete this playlist?",
                             "Are you sure you want to delete these %1 playlists?",
                             playlistList.count() )
                      , &dialog
                    );
        dialog.setButtonText( KDialog::Ok, i18n( "Yes, delete from database." ) );
        dialog.setMainWidget( &label );
        if( dialog.exec() != QDialog::Accepted )
            return;
    }

    foreach( Playlists::SqlPlaylistPtr sqlPlaylist, playlistList )
    {
        if( sqlPlaylist )
        {
            debug() << "deleting " << sqlPlaylist->name();
            sqlPlaylist->removeFromDb();
        }
    }
    reloadFromDb();
}

Playlists::PlaylistPtr
SqlUserPlaylistProvider::save( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    return save( tracks,
          QDateTime::currentDateTime().toString( "ddd MMMM d yy hh:mm") );
}

Playlists::PlaylistPtr
SqlUserPlaylistProvider::save( const Meta::TrackList &tracks, const QString& name )
{
    DEBUG_BLOCK
    debug() << "saving " << tracks.count() << " tracks to db with name" << name;
    Playlists::SqlPlaylistPtr sqlPlaylist = Playlists::SqlPlaylistPtr(
            new Playlists::SqlPlaylist( name, tracks,
                Playlists::SqlPlaylistGroupPtr(),
                this )
            );
    reloadFromDb();

    return Playlists::PlaylistPtr::dynamicCast( sqlPlaylist ); //assumes insertion in db was successful!
}

bool
SqlUserPlaylistProvider::import( const QString& fromLocation )
{
    DEBUG_BLOCK
    debug() << "importing playlist " << fromLocation;
    QString query = "SELECT id, parent_id, name, description, urlid FROM \
                playlists where urlid='%1';";
    SqlStorage *sql = CollectionManager::instance()->sqlStorage();
    if( !sql )
    {
        debug() << "No sql storage available!";
	    return false;
    }
    query = query.arg( sql->escape( fromLocation ) );
    QStringList result = sql->query( query );
    if( result.count() != 0 )
    {
        debug() << "Playlist was already imported";
        return false;
    }


    KUrl url( fromLocation );
    Playlists::Playlist* playlist = 0;
    Playlists::PlaylistFormat format = Playlists::getFormat( fromLocation );

    switch( format )
    {
        case Playlists::PLS:
            playlist = new Playlists::PLSPlaylist( url );
            break;
        case Playlists::M3U:
            playlist = new Playlists::M3UPlaylist( url );
            break;
        case Playlists::XSPF:
            playlist = new Playlists::XSPFPlaylist( url );
            break;

        default:
            debug() << "unknown type, cannot save playlist!";
            return false;
    }
    Meta::TrackList tracks = playlist->tracks();
    QString name = playlist->name().split('.')[0];
    debug() << name << QString(" has %1 tracks.").arg( tracks.count() );
    if( tracks.isEmpty() )
        return false;

    Playlists::SqlPlaylistPtr sqlPlaylist =
        Playlists::SqlPlaylistPtr( new Playlists::SqlPlaylist( playlist->name(), tracks,
                                                     Playlists::SqlPlaylistGroupPtr(),
                                                     this,
                                                     fromLocation )
                              );
    reloadFromDb();
    emit updated();

    return true;
}

void
SqlUserPlaylistProvider::reloadFromDb()
{
    DEBUG_BLOCK;
    m_root->clear();
    emit updated();
}

Playlists::SqlPlaylistGroupPtr
SqlUserPlaylistProvider::group( const QString &name )
{
    DEBUG_BLOCK
    Playlists::SqlPlaylistGroupPtr group;

    if( name.isEmpty() )
        return m_root;

    //clear the root first to force a reload.
    m_root->clear();

    foreach( const Playlists::SqlPlaylistGroupPtr &group, m_root->allChildGroups() )
    {
        debug() << group->name();
        if( group->name() == name )
        {
            debug() << "match";
            return group;
        }
    }

    debug() << "Creating a new group " << name;
    group = new Playlists::SqlPlaylistGroup( name, m_root, this );
    group->save();

    return group;
}

void
SqlUserPlaylistProvider::createTables()
{
    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    if( !sqlStorage )
    {
        debug() << "No SQL Storage available!";
        return;
    }
    sqlStorage->query( QString( "CREATE TABLE playlist_groups ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() + " ) ENGINE = MyISAM;" ) );
    sqlStorage->query( "CREATE INDEX parent_podchannel ON playlist_groups( parent_id );" );


    sqlStorage->query( QString( "CREATE TABLE playlists ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() +
            ", urlid " + sqlStorage->exactTextColumnType() + " ) ENGINE = MyISAM;" ) );
    sqlStorage->query( "CREATE INDEX parent_playlist ON playlists( parent_id );" );

    sqlStorage->query( QString( "CREATE TABLE playlist_tracks ("
            " id " + sqlStorage->idType() +
            ", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url " + sqlStorage->exactTextColumnType() +
            ", title " + sqlStorage->textColumnType() +
            ", album " + sqlStorage->textColumnType() +
            ", artist " + sqlStorage->textColumnType() +
            ", length INTEGER "
            ", uniqueid " + sqlStorage->textColumnType(128) + ") ENGINE = MyISAM;" ) );

    sqlStorage->query( "CREATE INDEX parent_playlist_tracks ON playlist_tracks( playlist_id );" );
    sqlStorage->query( "CREATE INDEX playlist_tracks_uniqueid ON playlist_tracks( uniqueid );" );
}

void
SqlUserPlaylistProvider::deleteTables()
{
    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();

    if( !sqlStorage )
    {
        debug() << "No SQL Storage available!";
        return;
    }

    sqlStorage->query( "DROP INDEX parent_podchannel ON playlist_groups;" );
    sqlStorage->query( "DROP INDEX parent_playlist ON playlists;" );
    sqlStorage->query( "DROP INDEX parent_playlist_tracks ON playlist_tracks;" );
    sqlStorage->query( "DROP INDEX playlist_tracks_uniqueid ON playlist_tracks;" );

    sqlStorage->query( "DROP TABLE playlist_groups;" );
    sqlStorage->query( "DROP TABLE playlists;" );
    sqlStorage->query( "DROP TABLE playlist_tracks;" );

}

void
SqlUserPlaylistProvider::checkTables()
{
    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    QStringList values;

    //Prevents amarok from crashing on bad DB
    if ( !sqlStorage )
	    return;

    values = sqlStorage->query( QString("SELECT version FROM admin WHERE component = '%1';").arg(sqlStorage->escape( key ) ) );
    
    if( values.isEmpty() )
    {
        //debug() << "creating Playlist Tables";
        createTables();

        sqlStorage->query( "INSERT INTO admin(component,version) "
                "VALUES('" + key + "'," + QString::number( USERPLAYLIST_DB_VERSION ) + ");" );
    }
    else
    {
        int dbVersion = values.at( 0 ).toInt();
        if ( dbVersion != USERPLAYLIST_DB_VERSION ) {
            //ah screw it, we do not have any stable releases of this out, so just redo the db. This wil also make sure that we do not
            //get duplicate playlists from files due to one having a urlid and the other not having one
            deleteTables();
            createTables();

            sqlStorage->query( "UPDATE admin SET version = '" + QString::number( USERPLAYLIST_DB_VERSION )  + "' WHERE component = '" + key + "';" );
        }
    }
}

Playlists::SqlPlaylistList
SqlUserPlaylistProvider::toSqlPlaylists( Playlists::PlaylistList playlists )
{
    Playlists::SqlPlaylistList sqlPlaylists;
    foreach( Playlists::PlaylistPtr playlist, playlists )
    {
        Playlists::SqlPlaylistPtr sqlPlaylist =
            Playlists::SqlPlaylistPtr::dynamicCast( playlist );
        if( !sqlPlaylist.isNull() )
            sqlPlaylists << sqlPlaylist;
    }
    return sqlPlaylists;
}

} //namespace Playlists

#include "SqlUserPlaylistProvider.moc"
