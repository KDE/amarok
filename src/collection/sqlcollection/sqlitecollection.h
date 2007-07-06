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

#ifndef AMAROK_COLLECTION_SQLITECOLLECTION_H
#define AMAROK_COLLECTION_SQLITECOLLECTION_H

#include "sqlcollection.h"

#include <QMutex>

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_context sqlite3_context;
typedef struct Mem sqlite3_value;

class SqliteCollection : public SqlCollection
{
    public:
        SqliteCollection( const QString &id, const QString &prettyName );
        virtual ~SqliteCollection();

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table );

        virtual QString type() const;

    private:
        static void sqlite_rand( sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/ );
        static void sqlite_power( sqlite3_context *context, int argc, sqlite3_value **argv );
        static void sqlite_like_new( sqlite3_context *context, int argc, sqlite3_value **argv );

        bool m_initialized;
        QMutex m_mutex;

        sqlite3* m_db;
};

#endif
