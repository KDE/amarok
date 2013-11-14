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

#define DEBUG_PREFIX "MySqlEmbeddedStorage"

#include "MySqlEmbeddedStorage.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "amarokconfig.h"

#include <QDir>
#include <QString>
#include <QMutexLocker>
#include <QThreadStorage>
#include <QVarLengthArray>
#include <QVector>

#include <mysql.h>

MySqlEmbeddedStorage::MySqlEmbeddedStorage( const QString &storageLocation )
    : MySqlStorage()
{
    m_debugIdent = "MySQLe";

    QString storagePath = storageLocation;
    QString databaseDir;
    if( storageLocation.isEmpty() )
    {
        storagePath = Amarok::saveLocation();
        databaseDir = Amarok::config( "MySQLe" ).readEntry( "data", QString(storagePath + "mysqle") );
    }
    else
    {
        QDir dir( storageLocation );
        dir.mkpath( "." );  //ensure directory exists
        databaseDir = dir.absolutePath() + QDir::separator() + "mysqle";
    }

    QVector<const char*> mysql_args;
    QByteArray dataDir = QString( "--datadir=%1" ).arg( databaseDir ).toLocal8Bit();
    mysql_args << "amarok"
               << dataDir.constData()
               // CAUTION: if we ever change the table type we will need to fix a number of MYISAM specific
               // functions, such as FULLTEXT indexing.
               << "--default-storage-engine=MyISAM"
               << "--innodb=OFF"
               << "--skip-grant-tables"
               << "--myisam-recover=FORCE"
               << "--key-buffer-size=16777216" // (16Mb)
               << "--character-set-server=utf8"
               << "--collation-server=utf8_bin";


    if( !QFile::exists( databaseDir ) )
    {
        QDir dir( databaseDir );
        dir.mkpath( "." );
    }

    int ret = mysql_library_init( mysql_args.size(), const_cast<char**>(mysql_args.data()), 0 );
    if( ret != 0 )
    {
        // it has no sense to call reportError here because m_db is not yet initialized
        QMutexLocker locker( &m_mutex );
        QString errorMessage( "GREPME " + m_debugIdent + " library initialization "
                              "failed, return code " + QString::number( ret ) );
        m_lastErrors.append( errorMessage );
        error() << errorMessage.toLocal8Bit().constData();
        error() << "mysqle arguments were:" << mysql_args;
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
