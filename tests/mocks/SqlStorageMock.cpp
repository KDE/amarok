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

#include "SqlStorageMock.h"

OrderedSqlStorageMock::OrderedSqlStorageMock( const QList<QPair<QString, QVariant> > &queries)
    : SqlStorage()
    , m_queries(queries)
{
}

OrderedSqlStorageMock::~OrderesSqlStorageMock()
{
}

bool
OrderedSqlStorageMock::allQueriesRun() const
{
    return m_queries.isEmpty();
}

QStringList
OrderedSqlStorageMock::query( const QString &query )
{
    QVERIFY( !m_queries.isEmpty() );
    QPair<QString, QVariant> pair = m_queries.takeFirst();
    QCOMPARE( query, pair.first );
    QVariant value = pair.second;
    QVERIFY(value.canConvert(QVariant::StringList));
    return value.toStringList();
}

int
OrderedSqlStorageMock::insert(const QString &statement, const QString &table)
{
    QVERIFY( !m_queries.isEmpty() );
    QPair<QString, QVariant> pair = m_queries.takeFirst();
    QCOMPARE( statement, pair.first );
    QVariant value = pair.second;
    QVERIFY(value.canConvert(QVariant::Int));
    return value.toInt();
}

RandomSqlStorageMock::RandomSqlStorageMock( const QVariantMap &queries )
    : SqlStorage()
    , m_queries()
{
    for( auto key : queries.keys() )
    {
        m_queries.insert( key.toLower(), queries.value( key ) );
    }
}

RandomSqlStorageMock::~RandomSqlStorageMock()
{
}

bool
RandomSqlStorageMock::allQueriesRun() const
{
    return m_queries.isEmpty();
}

QStringList
RandomSqlStorageMock::query( const QString &query )
{
    QVERIFY2( m_queries.contains( query.toLower() ), "Received an unknown query in query()" );
    QVariant value = m_queries.value( query );
    QVERIFY(value.canConvert(QVariant::StringList));
    return value.toStringList();
}

int
RandomSqlStorageMock::insert(const QString &statement, const QString &table)
{
    QVERIFY2( m_queries.contains( statement.toLower() ), "Received an unknown query in insert()" );
    QVariant value = m_queries.value( statement );
    QVERIFY(value.canConvert(QVariant::Int));
    return value.toInt();
}
