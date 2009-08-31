/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "SqlUserPlaylistProvider.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "meta/M3UPlaylist.h"
#include "meta/PLSPlaylist.h"
#include "meta/XSPFPlaylist.h"
#include "PlaylistFileSupport.h"
#include "SqlStorage.h"
#include "SvgHandler.h"
#include "UserPlaylistModel.h"

#include <KIcon>
#include <KInputDialog>
#include <KUrl>

#include <QAction>
#include <QMap>

static const int USERPLAYLIST_DB_VERSION = 2;
static const QString key("AMAROK_USERPLAYLIST");

typedef QMultiMap<Meta::PlaylistPtr, Meta::TrackPtr> PlaylistTrackMap;
Q_DECLARE_METATYPE( PlaylistTrackMap )

SqlUserPlaylistProvider::SqlUserPlaylistProvider()
    : UserPlaylistProvider()
    , m_renameAction( 0 )
    , m_deleteAction( 0 )
    , m_removeTrackAction( 0 )
{
    checkTables();
    m_root = Meta::SqlPlaylistGroupPtr( new Meta::SqlPlaylistGroup( "",
            Meta::SqlPlaylistGroupPtr() ) );
}

SqlUserPlaylistProvider::~SqlUserPlaylistProvider()
{
}

Meta::PlaylistList
SqlUserPlaylistProvider::playlists()
{
    Meta::PlaylistList playlists;
    foreach( Meta::SqlPlaylistPtr sqlPlaylist, m_root->allChildPlaylists() )
    {
        playlists << Meta::PlaylistPtr::staticCast( sqlPlaylist );
    }
    return playlists;
}

void
SqlUserPlaylistProvider::slotDelete()
{
    DEBUG_BLOCK

    //TODO FIXME Confirmation of delete
    foreach( Meta::PlaylistPtr playlist, The::userPlaylistModel()->selectedPlaylists() )
    {
        Meta::SqlPlaylistPtr sqlPlaylist =
                Meta::SqlPlaylistPtr::dynamicCast( playlist );
        if( sqlPlaylist )
        {
            debug() << "deleting " << sqlPlaylist->name();
            sqlPlaylist->removeFromDb();
        }
    }
    reloadFromDb();
}

void
SqlUserPlaylistProvider::slotRename()
{
    DEBUG_BLOCK
    //only one playlist can be selected at this point
    Meta::SqlPlaylistPtr playlist = selectedPlaylists().first();
    if( playlist.isNull() )
        return;
    //TODO: inline rename
    bool ok;
    const QString newName = KInputDialog::getText( i18n("Change playlist"),
                i18n("Enter new name for playlist:"), playlist->name(),
                                                   &ok );
    if ( ok )
        playlist->setName( newName.trimmed() );\
}

void
SqlUserPlaylistProvider::slotRemove()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    PlaylistTrackMap playlistMap = action->data().value<PlaylistTrackMap>();
    foreach( Meta::PlaylistPtr playlist, playlistMap.keys() )
        foreach( Meta::TrackPtr track, playlistMap.values( playlist ) )
            playlist->removeTrack( playlist->tracks().indexOf( track ) );

    //clear the data
    action->setData( QVariant() );
}

QList<QAction *>
SqlUserPlaylistProvider::playlistActions( Meta::PlaylistPtr playlist )
{
    Q_UNUSED( playlist );
    QList<QAction *> actions;

    m_selectedPlaylists.clear();
    m_selectedPlaylists << Meta::SqlPlaylistPtr::dynamicCast( playlist );

    if ( m_renameAction == 0 )
    {
        m_renameAction =  new QAction( KIcon( "media-track-edit-amarok" ), i18n( "&Rename" ), this );
        m_renameAction->setProperty( "popupdropper_svg_id", "edit" );
        connect( m_renameAction, SIGNAL( triggered() ), this, SLOT( slotRename() ) );
    }
    actions << m_renameAction;

    if ( m_deleteAction == 0 )
    {
        m_deleteAction = new QAction( KIcon( "media-track-remove-amarok" ), i18n( "&Delete" ), this );
        m_deleteAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_deleteAction, SIGNAL( triggered() ), SLOT( slotDelete() ) );
    }
    actions << m_deleteAction;

    return actions;
}

QList<QAction *>
SqlUserPlaylistProvider::trackActions( Meta::PlaylistPtr playlist, int trackIndex )
{
    Q_UNUSED( trackIndex );
    QList<QAction *> actions;

    if( m_removeTrackAction == 0 )
    {
        m_removeTrackAction = new QAction(
                    KIcon( "media-track-remove-amarok" ),
                    i18n( "Remove From Playlist" ),
                    this
                );
        m_removeTrackAction->setProperty( "popupdropper_svg_id", "delete" );
        connect( m_removeTrackAction, SIGNAL( triggered() ), SLOT( slotRemove() ) );
    }
    //Add the playlist/track combination to a QMultiMap that is stored in the action.
    //In the slot we use this data and use it to remove that track.
    PlaylistTrackMap playlistMap = m_removeTrackAction->data().value<PlaylistTrackMap>();
    playlistMap.insert( playlist, playlist->tracks()[trackIndex] );
    m_removeTrackAction->setData( QVariant::fromValue( playlistMap ) );

    actions << m_removeTrackAction;

    return actions;
}

Meta::PlaylistPtr
SqlUserPlaylistProvider::save( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    return save( tracks,
          QDateTime::currentDateTime().toString( "ddd MMMM d yy hh:mm") );
}

Meta::PlaylistPtr
SqlUserPlaylistProvider::save( const Meta::TrackList &tracks, const QString& name )
{
    DEBUG_BLOCK
    debug() << "saving " << tracks.count() << " tracks to db with name" << name;
    Meta::SqlPlaylistPtr sqlPlaylist = Meta::SqlPlaylistPtr( new Meta::SqlPlaylist( name, tracks,
            Meta::SqlPlaylistGroupPtr() ) );
    reloadFromDb();

    return Meta::PlaylistPtr::dynamicCast( sqlPlaylist ); //assumes insertion in db was successful!
}

bool
SqlUserPlaylistProvider::import( const QString& fromLocation )
{
    DEBUG_BLOCK
    debug() << "importing playlist " << fromLocation;
    QString query = "SELECT id, parent_id, name, description, urlid FROM \
                playlists where urlid='%1';";
    SqlStorage *sql = CollectionManager::instance()->sqlStorage();
    query = query.arg( sql->escape( fromLocation ) );
    QStringList result = sql->query( query );
    if( result.count() != 0 )
    {
        debug() << "Playlist was already imported";
        return false;
    }


    KUrl url( fromLocation );
    Meta::Playlist* playlist = 0;
    Meta::PlaylistFormat format = Meta::getFormat( fromLocation );

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
            debug() << "unknown type, cannot save playlist!";
            return false;
    }
    Meta::TrackList tracks = playlist->tracks();
    QString name = playlist->name().split('.')[0];
    debug() << name << QString(" has %1 tracks.").arg( tracks.count() );
    if( tracks.isEmpty() )
        return false;

    Meta::SqlPlaylistPtr sqlPlaylist =
        Meta::SqlPlaylistPtr( new Meta::SqlPlaylist( playlist->name(), tracks,
                                                     Meta::SqlPlaylistGroupPtr(), fromLocation ) );
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

Meta::SqlPlaylistGroupPtr
SqlUserPlaylistProvider::group( const QString &name )
{
    DEBUG_BLOCK
    Meta::SqlPlaylistGroupPtr group;

    if( name.isEmpty() )
        return m_root;

    //clear the root first to force a reload.
    m_root->clear();

    foreach( const Meta::SqlPlaylistGroupPtr &group, m_root->allChildGroups() )
    {
        debug() << group->name();
        if( group->name() == name )
        {
            debug() << "match";
            return group;
        }
    }

    debug() << "Creating a new group " << name;
    group = new Meta::SqlPlaylistGroup( name, m_root );
    group->save();

    return group;
}

void
SqlUserPlaylistProvider::createTables()
{
    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
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
    QStringList values = sqlStorage->query( QString("SELECT version FROM admin WHERE component = '%1';").arg(sqlStorage->escape( key ) ) );
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

Meta::SqlPlaylistList
SqlUserPlaylistProvider::toSqlPlaylists( Meta::PlaylistList playlists )
{
    Meta::SqlPlaylistList sqlPlaylists;
    foreach( Meta::PlaylistPtr playlist, playlists )
    {
        Meta::SqlPlaylistPtr sqlPlaylist =
            Meta::SqlPlaylistPtr::dynamicCast( playlist );
        if( !sqlPlaylist.isNull() )
            sqlPlaylists << sqlPlaylist;
    }
    return sqlPlaylists;
}

#include "SqlUserPlaylistProvider.moc"
