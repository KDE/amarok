/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "MySqlCollection.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "MySqlQueryMaker.h"

#include <klocale.h>

MySqlCollection::MySqlCollection( const QString &id, const QString &prettyName )
    : SqlCollection( id, prettyName )
{
    QString dbFile = Amarok::config( "Sqlite" ).readEntry( "location",
                                        Amarok::saveLocation() + "collection.db" );

    DEBUG_BLOCK

    debug() ;
    m_db = mysql_init(NULL);
    if (m_db)
    {
//         if ( config->username().isEmpty() )
//             pApp->slotConfigAmarok("MySql");

        if ( mysql_real_connect( m_db,
                 Amarok::config( "MySql" ).readEntry( "host" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "user" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "password" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "database" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "port" ).toInt(),
                                       NULL, CLIENT_COMPRESS ) )
        {
            m_initialized = true;

#if MYSQL_VERSION_ID >= 40113
            // now set the right charset for the connection
            QStringList my_qslist = query( "SHOW VARIABLES LIKE 'character_set_database'" );
            if( !my_qslist.isEmpty() && !mysql_set_character_set( m_db, const_cast<char *>( my_qslist[1].toLatin1().data() ) ) )
                //charset was updated
                debug() << "Connection Charset is now: " << my_qslist[1].toLatin1();
            else
                error() << "Failed to set database charset\n";
#endif

            m_db->reconnect = 1; //setting reconnect flag for newer mysqld
            m_connected = true;
        }
        else
        {
            if ( mysql_real_connect( m_db,
                 Amarok::config( "MySql" ).readEntry( "host" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "user" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "password" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "database" ).toLatin1(),
                 Amarok::config( "MySql" ).readEntry( "port" ).toInt(),
                                       NULL, CLIENT_COMPRESS ) )
            {
                if ( mysql_query(m_db,
                    QString( "CREATE DATABASE " + Amarok::config( "MySql" ).readEntry( "database" ) ).toLatin1() ) )
                    { m_connected = true; m_initialized = true; }
                else
                    { setMysqlError(); }
            }
            else
                setMysqlError();

        }
    }
    else
        error() << "Failed to allocate/initialize MySql struct\n";
    init();
}

MySqlCollection::~MySqlCollection()
{
    if ( m_db ) mysql_close( m_db );
}

QueryMaker*
MySqlCollection::queryMaker()
{
    return new MySqlQueryMaker( this );
}

QStringList MySqlCollection::query( const QString& statement )
{
    QStringList values;

    if ( !mysql_query( m_db, statement.toUtf8() ) )
    {
        MYSQL_RES* result;
        if ( ( result = mysql_use_result( m_db ) ) )
        {
            int number = mysql_field_count( m_db );
            MYSQL_ROW row;
            while ( ( row = mysql_fetch_row( result ) ) )
            {
                for ( int i = 0; i < number; i++ )
                {
                  values << QString::fromUtf8( (const char*)row[i] );
                }
            }
        }
        else
        {
            if ( mysql_field_count( m_db ) != 0 )
            {
//                 if ( !suppressDebug ) // FIXME
                    debug() << "MYSQL QUERY FAILED: " << mysql_error( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
                values = QStringList();
            }
        }
        mysql_free_result( result );
    }
    else
    {
//         if ( !suppressDebug ) // FIXME
            debug() << "MYSQL QUERY FAILED: " << mysql_error( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        values = QStringList();
    }

    return values;
}

int MySqlCollection::insert( const QString& statement, const QString& /* table */ )
{
    mysql_query( m_db, statement.toUtf8() );
    if ( mysql_errno( m_db ) )
    {
        debug() << "MYSQL INSERT FAILED: " << mysql_error( m_db ) << "\n" << "FAILED INSERT: " << statement;
    }
    return mysql_insert_id( m_db );
}

QString
MySqlCollection::type() const
{
    return "MySQL";
}

QString
MySqlCollection::escape( QString text ) const               //krazy:exclude=constref
{
    return text.replace("\\", "\\\\").replace( '\'', "''" );
}

void
MySqlCollection::setMysqlError()
{
    m_error = i18n("MySQL reported the following error:<br/>") + mysql_error(m_db)
            + i18n("<p>You can configure MySQL in the Collection section under Settings->Configure Amarok</p>");
}

QueryMaker*
MySqlQueryMaker::orderByRandom()
{
    d->queryOrderBy = " ORDER BY RAND()";
    return this;
}


