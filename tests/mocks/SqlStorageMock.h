/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef SQLSTORAGEMOCK_H
#define SQLSTORAGEMOCK_H

#include "core/collections/support/SqlStorage.h"

#include <QList>
#include <QPair>
#include <QStringList>
#include <QTest>
#include <QVariant>
#include <QVariantMap>

class OrderedSqlStorageMock
{
public:
    OrderedSqlStorageMock( const QList<QPair<QString, QVariant> > queries );
    virtual ~OrderesSqlStorageMock();

    virtual int sqlDatabasePriority() const { return 0; }

    virtual QString type() const { return "RandomSqlStorageMock"; }

    virtual QString escape( QString text ) const { return text; }

    virtual QStringList query( const QString &query );
    virtual int insert( const QString &statement, const QString &table );

    virtual QString boolTrue() const { return "1"; }
    virtual QString boolFalse() const { return "0"; }

    /**
        use this type for auto incrementing integer primary keys.
    */
    virtual QString idType() const { QFAIL( "idType not implemented" ); return QString(); }

    virtual QString textColumnType( int length = 255 ) const { QFAIL( "textColumnType not implemented" ); return QString(); }
    virtual QString exactTextColumnType( int length = 1000 ) const { QFAIL( "exactTextColumnType not implemented" ); return QString(); }
    //the below value may have to be decreased even more for different indexes; only time will tell
    virtual QString exactIndexableTextColumnType( int length = 324 ) const { QFAIL( "exactIdexableTextColumnType not implemented" ); return QString(); }
    virtual QString longTextColumnType() const { QFAIL( "longTextColumnType not implemented" ); return QString(); }
    virtual QString randomFunc() const { QFAIL( "ramdomFunc not implemented" ); return QString(); }

    //Mock specific method
    bool allQueriesRun() const;

private:
    QList<QPair<QString, QVariant> > m_queries;
};

class RandomSqlStorageMock : public SqlStorage
{
public:
    RandomSqlStorageMock( const QVariantMap &queries );
    virtual ~RandomSqlStorageMock();

    virtual int sqlDatabasePriority() const { return 0; }

    virtual QString type() const { return "RandomSqlStorageMock"; }

    virtual QString escape( QString text ) const { return text; }

    virtual QStringList query( const QString &query );
    virtual int insert( const QString &statement, const QString &table );

    virtual QString boolTrue() const { return "1"; }
    virtual QString boolFalse() const { return "0"; }

    /**
        use this type for auto incrementing integer primary keys.
    */
    virtual QString idType() const { QFAIL( "idType not implemented" ); return QString(); }

    virtual QString textColumnType( int length = 255 ) const { QFAIL( "textColumnType not implemented" ); return QString(); }
    virtual QString exactTextColumnType( int length = 1000 ) const { QFAIL( "exactTextColumnType not implemented" ); return QString(); }
    //the below value may have to be decreased even more for different indexes; only time will tell
    virtual QString exactIndexableTextColumnType( int length = 324 ) const { QFAIL( "exactIdexableTextColumnType not implemented" ); return QString(); }
    virtual QString longTextColumnType() const { QFAIL( "longTextColumnType not implemented" ); return QString(); }
    virtual QString randomFunc() const { QFAIL( "ramdomFunc not implemented" ); return QString(); }

    //Mock specific method
    bool allQueriesRun() const;

private:
    QVariantMap m_queries;

};

#endif // SQLSTORAGEMOCK_H
