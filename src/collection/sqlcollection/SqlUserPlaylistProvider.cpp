
void PlaylistBrowserNS::UserModel::createTables()
{
    DEBUG_BLOCK

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    sqlStorage->query( QString( "CREATE TABLE playlist_groups ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() + " );" ) );
    sqlStorage->query( "CREATE INDEX parent_podchannel ON playlist_groups( parent_id );" );


    sqlStorage->query( QString( "CREATE TABLE playlists ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() +
            ", urlid " + sqlStorage->exactTextColumnType() + " );" ) );
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
            ", uniqueid " + sqlStorage->textColumnType(128) + ");" ) );

    sqlStorage->query( "CREATE INDEX parent_playlist_tracks ON playlist_tracks( playlist_id );" );
    sqlStorage->query( "CREATE INDEX playlist_tracks_uniqueid ON playlist_tracks( uniqueid );" );



}

void PlaylistBrowserNS::UserModel::deleteTables()
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

void PlaylistBrowserNS::UserModel::checkTables()
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
    } else {

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

void
PlaylistBrowserNS::UserModel::reloadFromDb()
{
    DEBUG_BLOCK;
    reset();
    m_root->clear();
}