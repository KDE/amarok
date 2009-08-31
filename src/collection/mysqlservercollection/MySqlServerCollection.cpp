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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MySqlServerCollection.h"

#include "Amarok.h"
#include "Debug.h"
#include "amarokconfig.h"

#include <QMutexLocker>
#include <QThreadStorage>
#include <QVarLengthArray>

#include <mysql.h>

AMAROK_EXPORT_PLUGIN( MySqlServerCollectionFactory )

void
MySqlServerCollectionFactory::init()
{
    Amarok::Collection* collection;

    collection = new MySqlServerCollection( "serverCollection", i18n( "Local Collection (on %1)").arg( Amarok::config( "MySQL" ).readEntry( "Host" ) ) );

    emit newCollection( collection );
}

MySqlServerCollection::MySqlServerCollection( const QString &id, const QString &prettyName )
    : MySqlCollection( id, prettyName )
{
    DEBUG_BLOCK
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

    if( !mysql_real_connect( m_db,
                Amarok::config( "MySQL" ).readEntry( "Host" ).toUtf8(),
                Amarok::config( "MySQL" ).readEntry( "User" ).toUtf8(),
                Amarok::config( "MySQL" ).readEntry( "Password" ).toUtf8(),
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
        QString databaseName = Amarok::config( "MySQL" ).readEntry( "Database", "amarok" );
        if( mysql_query( m_db, QString( "SET NAMES 'utf8'" ).toUtf8() ) )
            reportError( "SET NAMES 'utf8' died" );
        if( mysql_query( m_db, QString( "CREATE DATABASE IF NOT EXISTS %1 DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci" ).arg( databaseName ).toUtf8() ) )
            reportError( QString( "Could not create %1 database" ).arg( databaseName ) );
        if( mysql_query( m_db, QString( "ALTER DATABASE %1 DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci" ).arg( databaseName ).toUtf8() ) )
            reportError( "Could not alter database charset/collation" );
        if( mysql_query( m_db, QString( "USE %1" ).arg( databaseName ).toUtf8() ) )
            reportError( "Could not select database" );

        debug() << "Connected to MySQL server" << mysql_get_server_info( m_db );
    }

    MySqlCollection::initThreadInitializer();
    init();
}

MySqlServerCollection::~MySqlServerCollection()
{
    DEBUG_BLOCK
}

QString
MySqlServerCollection::type() const
{
    return "MySQL";
}

#include "MySqlServerCollection.moc"

