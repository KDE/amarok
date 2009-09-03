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

#ifndef AMAROK_COLLECTION_MYSQLCOLLECTION_H
#define AMAROK_COLLECTION_MYSQLCOLLECTION_H

#include "sqlcollection/SqlCollection.h"

#include <QMutex>

struct st_mysql;
typedef struct st_mysql MYSQL;

/**
 * Implements a SqlCollection using a MySQL backend
 */
class MySqlCollection: public SqlCollection
{
    Q_OBJECT

    public:
        MySqlCollection( const QString &id, const QString &prettyName );
        virtual ~MySqlCollection();

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table );

        virtual QString escape( QString text ) const;
        virtual QString randomFunc() const;

        virtual QString type() const = 0;

    protected:
        void reportError( const QString& message );

        void initThreadInitializer();

        MYSQL* m_db;
        QMutex m_mutex;
};

#endif
