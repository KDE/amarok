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

#ifndef AMAROK_STORAGE_MYSQLSTORAGE_H
#define AMAROK_STORAGE_MYSQLSTORAGE_H

#include "core/storage/SqlStorage.h"
#include <mysql.h>


#include <QRecursiveMutex>
#include <QString>

#ifdef Q_WS_WIN
  #include <winsock2.h>
#endif

#if !defined(MARIADB_BASE_VERSION) && MYSQL_VERSION_ID >= 80000
using my_bool = bool;
#endif

/**
 * Implements a SqlStorage using a MySQL backend
 */
class MySqlStorage: public SqlStorage
{
    public:
        MySqlStorage();
        ~MySqlStorage() override;

        QStringList query( const QString &query ) override;
        int insert( const QString &statement, const QString &table = QString() ) override;

        QString escape( const QString &text ) const override;
        QString randomFunc() const override;

        QString boolTrue() const override;
        QString boolFalse() const override;
        QString idType() const override;
        QString textColumnType( int length = 255 ) const override;
        QString exactTextColumnType( int length = 1000 ) const override;
        //the below value may have to be decreased even more for different indexes; only time will tell
        QString exactIndexableTextColumnType( int length = 324 ) const override;
        QString longTextColumnType() const override;

        /** Returns a list of the last sql errors.
            The list might not include every one error if the number
            is beyond a sensible limit.
        */
        QStringList getLastErrors() const override;

        /** Clears the list of the last errors. */
        void clearLastErrors() override;

    protected:
        /** Adds an error message to the m_lastErrors.
         *
         *  Adds a message including the mysql error number and message
         *  to the last error messages.
         *  @param message Usually the query statement being executed.
         */
        void reportError( const QString &message );

        void initThreadInitializer();

        /** Sends the first sql commands to setup the connection.
         *
         *  Sets things like the used database and charset.
         *  @returns false if something fatal was wrong.
         */
        bool sharedInit( const QString &databaseName );

        MYSQL* m_db;

        /** Mutex protecting the m_lastErrors list */
        mutable QRecursiveMutex m_mutex;

        QString m_debugIdent;
        QStringList m_lastErrors;
};

#endif
