/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef SQLCOLLECTION__DBUS_HANDLER_H
#define SQLCOLLECTION__DBUS_HANDLER_H

#include <QObject>
#include <QDBusArgument>

class SqlCollection;

class SqlCollectionDBusHandler : public QObject
{
    Q_OBJECT
    Q_CLASSINFO( "SQL Collection D-Bus Interface", "org.kde.amarok.SqlCollection" )
    public:
        SqlCollectionDBusHandler( QObject *parent );

        void setCollection( SqlCollection *collection ) { m_collection = collection; }

    public slots:
        bool isDirInCollection( const QString& path );
    
    private:
    SqlCollection *m_collection;
};


#endif
