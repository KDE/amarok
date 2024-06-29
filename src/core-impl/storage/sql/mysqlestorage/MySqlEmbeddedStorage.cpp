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

#include <amarokconfig.h>
#include <core/support/Amarok.h>
#include <core/support/Debug.h>

#include <QDir>
#include <QVarLengthArray>
#include <QVector>
#include <QAtomicInt>

#include <mysql.h>

/** number of times the library is used.
 */
static QAtomicInt libraryInitRef;

MySqlEmbeddedStorage::MySqlEmbeddedStorage()
    : MySqlStorage()
{
    m_debugIdent = QStringLiteral("MySQLe");
}

bool
MySqlEmbeddedStorage::init( const QString &storageLocation )
{
    // -- figuring out and setting the database path.
    QString storagePath = storageLocation;
    QString databaseDir;
    // TODO: the following logic is not explained in the comments.
    //  tests use a different directory then the real run
    if( storagePath.isEmpty() )
    {
        storagePath = Amarok::saveLocation();
        databaseDir = Amarok::config( QStringLiteral("MySQLe") ).readEntry( "data", QString(storagePath + QStringLiteral("mysqle")) );
    }
    else
    {
        QDir dir( storagePath );
        dir.mkpath( QStringLiteral(".") );  //ensure directory exists
        databaseDir = dir.absolutePath() + QDir::separator() + QStringLiteral("mysqle");
    }

    QVector<const char*> mysql_args;
    QByteArray dataDir = QStringLiteral( "--datadir=%1" ).arg( databaseDir ).toLocal8Bit();
    mysql_args << "amarok"
               << dataDir.constData()
               // CAUTION: if we ever change the table type we will need to fix a number of MYISAM specific
               // functions, such as FULLTEXT indexing.
               << "--default-storage-engine=MyISAM"
               << "--innodb=OFF"
               << "--skip-grant-tables"
#if (defined(MYSQL_VERSION_ID)) && (MYSQL_VERSION_ID >= 50700)
               << "--myisam-recover-options=FORCE"
#else
               << "--myisam-recover=FORCE"
#endif
               << "--key-buffer-size=16777216" // (16Mb)
               << "--character-set-server=utf8"
               << "--collation-server=utf8_bin";


    if( !QFile::exists( databaseDir ) )
    {
        QDir dir( databaseDir );
        dir.mkpath( QStringLiteral(".") );
    }

    // -- initializing the library
    // we only need to do this once
    if( !libraryInitRef.fetchAndAddOrdered( 1 ) )
    {
        int ret = mysql_library_init( mysql_args.size(), const_cast<char**>(mysql_args.data()), nullptr );
        if( ret != 0 )
        {
            // mysql sources show that there is only 0 and 1 as return code
            // and it can only fail because of memory or thread issues.
            reportError( QStringLiteral("library initialization "
                         "failed, return code ") + QString::number( ret ) );
            libraryInitRef.deref();
            return false;
        }
    }

    m_db = mysql_init( nullptr );
    if( !m_db )
    {
        reportError( QStringLiteral("call to mysql_init") );
        return false;
    }

    if( mysql_options( m_db, MYSQL_READ_DEFAULT_GROUP, "amarokclient" ) )
        reportError( QStringLiteral("Error setting options for READ_DEFAULT_GROUP") );
    if( mysql_options( m_db, MYSQL_OPT_USE_EMBEDDED_CONNECTION, nullptr ) )
        reportError( QStringLiteral("Error setting option to use embedded connection") );

    if( !mysql_real_connect( m_db, nullptr,nullptr,nullptr, nullptr, 0,nullptr, 0 ) )
    {
        error() << "Could not connect to mysql embedded!";
        reportError( QStringLiteral("call to mysql_real_connect") );
        mysql_close( m_db );
        m_db = nullptr;
        return false;
    }

    if( !sharedInit( QLatin1String("amarok") ) )
    {
        // if sharedInit fails then we can usually not switch to the correct database
        // sharedInit already reports errors.
        mysql_close( m_db );
        m_db = nullptr;
        return false;
    }

    MySqlStorage::initThreadInitializer();

    return true;
}

MySqlEmbeddedStorage::~MySqlEmbeddedStorage()
{
    if( m_db )
    {
        mysql_close( m_db );
        libraryInitRef.deref();
    }
}

