/****************************************************************************************
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

#include "MySqlServerTester.h"

#include "Debug.h"
#include <mysql.h>

bool MySqlServerTester::testSettings( const QString &host, const QString &user, const QString &password, int port )
{
    DEBUG_BLOCK
    if( mysql_library_init( 0, NULL, NULL ) )
    {
        error() << "MySQL library initialization failed!";
        return false;
    }

    MYSQL* db = mysql_init( NULL );

    if( !db )
    {
        error() << "MySQL initialization failed";
        return false;
    }

    if( !mysql_real_connect( db, host.toUtf8(), user.toUtf8(), password.toUtf8(), NULL, port, NULL, CLIENT_COMPRESS ) )
    {
        mysql_close( db );
        db = 0;
        return false;
    }
    
    mysql_close( db );
    db = 0;
    return true;
}


