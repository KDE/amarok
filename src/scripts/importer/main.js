/**************************************************************************
*   Amarok 2 script to import Amarok 1.4 data                             *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Seb Ruiz <ruiz@kde.org>                                      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
**************************************************************************/

// "Importer" is ambiguous here. This means the QtSql importer.
Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.sql" );
Importer.load( "qtscript_debug/debug" );

Debug.initialize();
Debug.app_name = "Importer";



Debug.debug( "Connecting to Amarok 1.4 database" );
var db = QSqlDatabase.addDatabase( "QSQLITE", "a1" );
db.setDatabaseName( "/home/sebr/collection.db" );
db.open();

Debug.debug( "Fetching devices from Amarok 1.4" );
query = db.exec( "SELECT id, lastmountpoint FROM devices" );

while( query.next() )
{
    id = query.value(0).toString();
    lmp = query.value(1).toString();
    print( id + " : " + lmp );
}


function onConfigure()
{
    Amarok.alert( "Here database connectivity options will be placed." );
}

Amarok.configured.connect( onConfigure );

