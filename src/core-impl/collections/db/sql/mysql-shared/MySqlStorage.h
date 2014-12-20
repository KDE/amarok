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

#ifndef AMAROK_COLLECTION_MYSQLSTORAGE_H
#define AMAROK_COLLECTION_MYSQLSTORAGE_H

#include "core/collections/support/SqlStorage.h"


#include <QMutex>
#include <QString>

#ifdef Q_WS_WIN
  #include <winsock2.h>
#endif

struct st_mysql;
typedef struct st_mysql MYSQL;

/**
 * Implements a SqlCollection using a MySQL backend
 */
class MySqlStorage: public SqlStorage
{
    public:
        MySqlStorage();
        virtual ~MySqlStorage();

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table = QString() );

        virtual QString escape( const QString &text ) const;
        virtual QString randomFunc() const;

        virtual QString boolTrue() const;
        virtual QString boolFalse() const;
        virtual QString idType() const;
        virtual QString textColumnType( int length ) const;
        virtual QString exactTextColumnType( int length ) const;
        //the below value may have to be decreased even more for different indexes; only time will tell
        virtual QString exactIndexableTextColumnType( int length ) const;
        virtual QString longTextColumnType() const;

        virtual QString type() const = 0;

        /** Returns a list of the last sql errors.
            The list might not include every one error if the number
            is beyond a sensible limit.
        */
        QStringList getLastErrors() const;

        /** Clears the list of the last errors. */
        void clearLastErrors();

    protected:
        void reportError( const QString &message );

        void initThreadInitializer();
        void sharedInit( const QString &databaseName );

        MYSQL* m_db;

        mutable QMutex m_mutex;

        QString m_debugIdent;
        QStringList m_lastErrors;
};

#endif
