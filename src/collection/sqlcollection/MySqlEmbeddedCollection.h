/*
 *  Copyright (c) 2008 Edward Toroshchin <edward.hades@gmail.com>
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

#ifndef AMAROK_COLLECTION_MYSQLEMBEDDEDCOLLECTION_H
#define AMAROK_COLLECTION_MYSQLEMBEDDEDCOLLECTION_H

#include "SqlCollection.h"

#include <QMutex>

class st_mysql;
typedef struct st_mysql MYSQL;
/**
 * Implements a SqlCollection using a MySQL Embedded Server
 */
class MySqlEmbeddedCollection : public SqlCollection
{
    Q_OBJECT
    public:
        MySqlEmbeddedCollection( const QString &id, const QString &prettyName );
        virtual ~MySqlEmbeddedCollection();

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table );

        virtual QString escape( QString text ) const;
        virtual QString randomFunc() const;

        virtual QString type() const;

    private:
        void reportError( const QString& message );

        MYSQL* m_db;
        QMutex m_mutex;
};

#endif
