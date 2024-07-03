/****************************************************************************************
 * Copyright (c) 2008 Edward Toroshchin <edward.hades@gmail.com>                        *
 * Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>                                  *
 * Copyright (c) 2012 Lachlan Dufton <dufton@gmail.com>                                 *
 * Copyright (c) 2014 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "MySqlServerStorage"

#include "MySqlServerStorage.h"

#include <core/support/Debug.h>

#include <QAtomicInt>

#include <mysql.h>

/** number of times the library is used.
 */
static QAtomicInt libraryInitRef;

MySqlServerStorage::MySqlServerStorage()
    : MySqlStorage()
{
    m_debugIdent = QStringLiteral("MySQL-server");
}

bool
MySqlServerStorage::init( const QString &host, const QString &user, const QString &password, int port, const QString &databaseName )
{
    DEBUG_BLOCK

    // -- initializing the library
    // we only need to do this once
    if( !libraryInitRef.fetchAndAddOrdered( 1 ) )
    {
        int ret = mysql_library_init( 0, nullptr, nullptr );
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

    //first here, the right way for >= 5.1.6
    my_bool reconnect = true;
    if( mysql_options( m_db, MYSQL_OPT_RECONNECT, &reconnect ) )
        reportError( QStringLiteral("Asking for automatic reconnect did not succeed!") );
    else
        debug() << "Automatic reconnect successfully activated";

    debug() << "Connecting to mysql server " << user << "@" << host << ":" << port;
    if( !mysql_real_connect( m_db,
                host.toUtf8().constData(),
                user.toUtf8().constData(),
                password.toUtf8().constData(),
                nullptr,
                port,
                nullptr,
                CLIENT_COMPRESS )
        )
    {
        reportError( QStringLiteral("call to mysql_real_connect") );
        mysql_close( m_db );
        m_db = nullptr;
        return false;
    }

    //but in versions prior to 5.1.6, have to call it after every real_connect
    reconnect = true;
    if( mysql_options( m_db, MYSQL_OPT_RECONNECT, &reconnect ) )
        reportError( QStringLiteral("Asking for automatic reconnect did not succeed!") );
    else
        debug() << "Automatic reconnect successfully activated";

    m_databaseName = databaseName; // store it when we need it later for reconnect
    if( !sharedInit( databaseName ) )
    {
        // if sharedInit fails then we can usually not switch to the correct database
        // sharedInit already reports errors.
        mysql_close( m_db );
        m_db = nullptr;
        return false;
    }

    MySqlServerStorage::initThreadInitializer();
    return true;
}

MySqlServerStorage::~MySqlServerStorage()
{
    DEBUG_BLOCK

    if( m_db )
    {
        mysql_close( m_db );
        libraryInitRef.deref();
    }
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
        reportError( QStringLiteral("mysql_ping failed!") );
        return QStringList();
    }

    if( tid != mysql_thread_id( m_db ) )
    {
        debug() << "NOTE: MySQL server had gone away, ping reconnected it";
        if( mysql_query( m_db, QStringLiteral( "SET NAMES 'utf8'" ).toUtf8().constData() ) )
            reportError( QStringLiteral("SET NAMES 'utf8' died") );
        if( mysql_query( m_db, QStringLiteral( "USE %1" ).arg( m_databaseName ).toUtf8().constData() ) )
            reportError( QStringLiteral("Could not select database") );
    }


    return MySqlStorage::query( query );
}


