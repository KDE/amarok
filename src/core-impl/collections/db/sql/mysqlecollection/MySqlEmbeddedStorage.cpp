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

#include "MySqlEmbeddedStorage.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "amarokconfig.h"

#include <QDir>
#include <QString>
#include <QMutexLocker>
#include <QThreadStorage>
#include <QVarLengthArray>

#include <mysql.h>

MySqlEmbeddedStorage::MySqlEmbeddedStorage( const QString &storageLocation )
    : MySqlStorage()
{
    m_debugIdent = "MySQLe";

    QString storagePath = storageLocation;
    QString defaultsFile;
    QString databaseDir;
    if( storageLocation.isEmpty() )
    {
        storagePath = Amarok::saveLocation();
        defaultsFile = Amarok::config( "MySQLe" ).readEntry( "config", QString(storagePath + "my.cnf") );
        databaseDir = Amarok::config( "MySQLe" ).readEntry( "data", QString(storagePath + "mysqle") );
    }
    else
    {
        QDir dir( storageLocation );
        dir.mkpath( "." );  //ensure directory exists
        defaultsFile = QDir::cleanPath( dir.absoluteFilePath( "my.cnf" ) );
        databaseDir = dir.absolutePath() + QDir::separator() + "mysqle";
    }

    if( !Amarok::config( "MySQLe" ).readEntry( "keepUserMyCnf", false ) )
    {
        QFile df( defaultsFile );
        if ( !df.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
            error() << "Unable to open " << defaultsFile << " for writing.";
            reportError( "init" );
        }
        QTextStream out( &df );
        out << "[embedded]" << endl;
        out << "datadir = " << databaseDir.toLocal8Bit() << endl;
        // CAUTION: if we ever change the table type we will need to fix a number of MYISAM specific
        // functions, such as FULLTEXT indexing.
        out << "default-storage-engine = MyISAM" << endl;
        out << "loose-innodb = 0" << endl;
        out << "skip-grant-tables = 1" << endl;
        out << "myisam-recover = FORCE" << endl;
        out << "key_buffer_size = 16777216" << endl; // (16Mb)
        out << "character-set-server = utf8" << endl;
        out << "collation-server = utf8_bin" << endl;
        //If the file is world-writable MySQL won't even read it
        df.setPermissions( QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther );
        df.close();
    }

    if( !QFile::exists( databaseDir ) )
    {
        QDir dir( databaseDir );
        dir.mkpath( "." );
    }

    setenv( "MYSQL_HOME", storagePath.toLocal8Bit().data(), 1 );
    setenv( "DEFAULT_HOME_ENV", storagePath.toLocal8Bit().data(), 1 );
    char *args[] = { (char*) "amarok" };
    if( mysql_library_init( 1 , args, 0 ) != 0 )
    {
        error() << "MySQL library initialization failed.";
        reportError( "init" );
        return;
    }

    m_db = mysql_init( NULL );

    if( !m_db )
    {
        error() << "MySQLe initialization failed";
        return;
    }

    if( mysql_options( m_db, MYSQL_READ_DEFAULT_GROUP, "amarokclient" ) )
        reportError( "Error setting options for READ_DEFAULT_GROUP" );
    if( mysql_options( m_db, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL ) )
        reportError( "Error setting option to use embedded connection" );

    if( !mysql_real_connect( m_db, NULL,NULL,NULL, 0, 0,NULL, 0 ) )
    {
        error() << "Could not connect to mysql!";
        reportError( "na" );
        mysql_close( m_db );
        m_db = 0;
    }
    else
    {
        sharedInit( "amarok" );
        debug() << "Connected to MySQL server" << mysql_get_server_info( m_db );
    }

    MySqlStorage::initThreadInitializer();
}

MySqlEmbeddedStorage::~MySqlEmbeddedStorage()
{}

QString
MySqlEmbeddedStorage::type() const
{
    return "MySQLe";
}
