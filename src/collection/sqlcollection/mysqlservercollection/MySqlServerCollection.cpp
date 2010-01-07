/****************************************************************************************
 * Copyright (c) 2008 Edward Toroshchin <edward.hades@gmail.com>                        *
 * Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>                                  *
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

#include "MySqlServerCollection.h"

#include "Amarok.h"
#include "Debug.h"
#include "amarokconfig.h"
#include "SqlCollection.h"
#include "SqlCollectionFactory.h"

#include <QMutexLocker>
#include <QThreadStorage>
#include <QVarLengthArray>

#include <mysql.h>

AMAROK_EXPORT_COLLECTION( MySqlServerCollectionFactory, mysqlservercollection )

void
MySqlServerCollectionFactory::init()
{
    SqlCollectionFactory fac;
    SqlStorage *storage = new MySqlServerStorage();
    SqlCollection *collection = fac.createSqlCollection( "localCollection", i18n( "Local Collection" ), storage );

    emit newCollection( collection );
}

MySqlServerStorage::MySqlServerStorage()
    : MySqlStorage()
{
    DEBUG_BLOCK

    m_debugIdent = "MySQL-server";

    if( mysql_library_init( 0, NULL, NULL ) )
    {
        error() << "MySQL library initialization failed!";
        return;
    }

    m_db = mysql_init( NULL );

    if( !m_db )
    {
        error() << "MySQL initialization failed";
        return;
    }

    //first here, the right way for >= 5.1.6

    my_bool reconnect = true;
    if( mysql_options( m_db, MYSQL_OPT_RECONNECT, &reconnect ) )
        reportError( "Asking for automatic reconnect did not succeed!" );
    else
        debug() << "Automatic reconnect successfully activated";

    if( !mysql_real_connect( m_db,
                Amarok::config( "MySQL" ).readEntry( "Host", "localhost" ).toUtf8(),
                Amarok::config( "MySQL" ).readEntry( "User", "amarokuser" ).toUtf8(),
                Amarok::config( "MySQL" ).readEntry( "Password", "" ).toUtf8(),
                NULL,
                Amarok::config( "MySQL" ).readEntry( "Port", "3306" ).toInt(),
                NULL,
                CLIENT_COMPRESS )
        )
    {
        debug() << "connection to mysql failed";
        error() << "Could not connect to mysql!";
        reportError( "na" );
        mysql_close( m_db );
        m_db = 0;
    }
    else
    {
        //but in versions prior to 5.1.6, have to call it after every real_connect
        my_bool reconnect = true;
        if( mysql_options( m_db, MYSQL_OPT_RECONNECT, &reconnect ) )
            reportError( "Asking for automatic reconnect did not succeed!" );
        else
            debug() << "Automatic reconnect successfully activated";

        QString databaseName = Amarok::config( "MySQL" ).readEntry( "Database", "amarokdb" );
        sharedInit( databaseName );
        debug() << "Connected to MySQL server" << mysql_get_server_info( m_db );
    }

    MySqlServerStorage::initThreadInitializer();
}

MySqlServerStorage::~MySqlServerStorage()
{
    DEBUG_BLOCK
}

QString
MySqlServerStorage::type() const
{
    return "MySQL";
}

QStringList
MySqlServerStorage::query( const QString &query )
{
    MySqlStorage::initThreadInitializer();
    QMutexLocker locker( &m_mutex );
    if( !m_db )
    {
        error() << "Tried to query an uninitialized m_db!";
        return QStringList();
    }

    unsigned long tid = mysql_thread_id( m_db );

    int res = mysql_ping( m_db );
    if( res )
    {
        reportError( "mysql_ping failed!" );
        return QStringList();
    }

    if( tid != mysql_thread_id( m_db ) )
    {
        debug() << "NOTE: MySQL server had gone away, ping reconnected it";
        QString databaseName = Amarok::config( "MySQL" ).readEntry( "Database", "amarokdb" );
        if( mysql_query( m_db, QString( "SET NAMES 'utf8'" ).toUtf8() ) )
            reportError( "SET NAMES 'utf8' died" );
        if( mysql_query( m_db, QString( "USE %1" ).arg( databaseName ).toUtf8() ) )
            reportError( "Could not select database" );
    }


    return MySqlStorage::query( query );
}

#include "MySqlServerCollection.moc"

