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

#ifndef AMAROK_COLLECTION_MYSQLCOLLECTION_H
#define AMAROK_COLLECTION_MYSQLCOLLECTION_H

#include "SqlCollection.h"
#include <mysql/mysql.h>
#include <mysql/mysql_version.h>

class MySqlCollection : public SqlCollection
{
    public:
        MySqlCollection( const QString &id, const QString &prettyName );
        virtual ~MySqlCollection();

        virtual QueryMaker* queryMaker();

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table );

        virtual QString type() const;

        virtual QString escape( QString text ) const;
    private:
        bool m_initialized;

        void setMysqlError();
        MYSQL* m_db;
        bool m_connected;
        QString m_error;
};

#endif
