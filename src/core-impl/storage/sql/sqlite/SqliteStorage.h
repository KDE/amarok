/****************************************************************************************
 * Copyright (c) 2025 Amarok Team <amarok@kde.org>                                 *
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

#ifndef AMAROK_STORAGE_SQLITE_STORAGE_H
#define AMAROK_STORAGE_SQLITE_STORAGE_H

#include "core/storage/SqlStorage.h"

#include <QMutex>
#include <QRecursiveMutex>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

class SqliteStorage : public SqlStorage
{
public:
    SqliteStorage();
    ~SqliteStorage() override;

    bool init( const QString &dbPath );

    QString databaseName() const override;

    QStringList query( const QString &query ) override;
    int insert( const QString &statement, const QString &table = QString() ) override;

    QString escape( const QString &text ) const override;
    QString randomFunc() const override;
    QString sqlCreateTableOptions() const override;
    bool supportsPrefixIndexes() const override;

    QString boolTrue() const override;
    QString boolFalse() const override;
    QString idType() const override;
    QString textColumnType( int length = 255 ) const override;
    QString exactTextColumnType( int length = 1000 ) const override;
    QString exactIndexableTextColumnType( int length = 324 ) const override;
    QString longTextColumnType() const override;

    bool isMySQL() const override;

    QStringList getLastErrors() const override;
    void clearLastErrors() override;

private:
    void reportError( const QString &message );

    QSqlDatabase m_db;
    mutable QRecursiveMutex m_mutex;

    QString m_databaseName;
    QString m_lastError;
    QStringList m_lastErrors;
};

#endif