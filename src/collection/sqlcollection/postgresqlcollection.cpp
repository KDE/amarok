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

#include "postgresqlcollection.h"

#include "debug.h"
#include "postgresqlquerymaker.h"

#include <libpq-fe.h>

#define SHOW_DEBUG false

PostgreSqlCollection::PostgreSqlCollection( const QString &id, const QString &prettyName )
    : SqlCollection( id, prettyName )
{
    QString host = AmarokConfig::postgresqlHost();
    QString port = QString::number( AmarokConfig::postgresqlPort() );
    QString name = AmarokConfig::postgresqlDbName();
    QString user = AmarokConfig::postgresqlUser();
    QString passwd = Amarok::config( "Postgresql" ).readEntry( "PostgresqlPassword" );
    QString conninfo;
    debug() << k_funcinfo;

//     if ( config->username().isEmpty() )
//         pApp->slotConfigAmarok("Postgresql");

    conninfo = "host='" + host +
      "' port=" + port +
      " dbname='" + name +
      "' user='" + user +
      "' password='" + passwd + '\'';

    m_db = PQconnectdb( conninfo.toUtf8() );

    if (!m_db)
    {
        debug() << "POSTGRESQL CONNECT FAILED: " << PQerrorMessage( m_db ) << "\n";
        error() << "Failed to allocate/initialize Postgresql struct\n";
        setPostgresqlError();
        return;
    }

    if (PQstatus(m_db) != CONNECTION_OK)
    {
        debug() << "POSTGRESQL CONNECT FAILED: " << PQerrorMessage( m_db ) << "\n";
        error() << "Failed to allocate/initialize Postgresql struct\n";
        setPostgresqlError();
        PQfinish(m_db);
        m_db = NULL;
        return;
    }

    m_initialized = true;
    m_connected = true;
}

PostgreSqlCollection::~PostgreSqlCollection()
{
    if ( m_db ) PQfinish( m_db );
}

QueryMaker*
PostgreSqlCollection::queryMaker()
{
    return new PostgreSqlQueryMaker( this );
}

QString
PostgreSqlCollection::type() const
{
    return "PostgreSQL";
}

QString
PostgreSqlCollection::idType() const
{
    return " SERIAL PRIMARY KEY";
}

QStringList
PostgresqlCollection::query( const QString& statement )
{
    QStringList values;
    PGresult* result;
    ExecStatusType status;

    result = PQexec(m_db, statement.toUtf8());
    if (result == NULL)
    {
        if ( SHOW_DEBUG )
            debug() << "POSTGRESQL QUERY FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        return values;
    }

    status = PQresultStatus(result);
    if ((status != PGRES_COMMAND_OK) && (status != PGRES_TUPLES_OK))
    {
        if ( SHOW_DEBUG )
            debug() << "POSTGRESQL QUERY FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED QUERY: " << statement << "\n";
        PQclear(result);
        return values;
    }

    int cols = PQnfields( result );
    int rows = PQntuples( result );
    QMap<int, bool> discardCols;
    for(int col=0; col< cols; col++) {
        if (QString(PQfname(result, col)) == QString("__discard") || QString(PQfname(result, col)) == QString("__random"))
        {
            discardCols[col] = true;
        }
    }

    for(int row=0; row< rows; row++)
    {
        for(int col=0; col< cols; col++)
        {
            if (discardCols[col]) continue;

            values << QString::fromUtf8(PQgetvalue(result, row, col));
        }
    }

    PQclear(result);

    return values;
}


int
PostgresqlCollection::insert( const QString& statement, const QString& table )
{
    PGresult* result;
    ExecStatusType status;
    QString curvalSql;
    int id;

    result = PQexec(m_db, statement.toUtf8());
    if (result == NULL)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << statement << "\n";
        return 0;
    }

    status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << statement << "\n";
        PQclear(result);
        return 0;
    }
    PQclear(result);

    if (table == NULL) return 0;

    QString _table = table;
    if (table.find("_temp") > 0) _table = table.left(table.find("_temp"));

    curvalSql = QString("SELECT currval('%1_seq');").arg(_table);
    result = PQexec(m_db, curvalSql.toUtf8());
    if (result == NULL)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << curvalSql << "\n";
        return 0;
    }

    status = PQresultStatus(result);
    if (status != PGRES_TUPLES_OK)
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << curvalSql << "\n";
        PQclear(result);
        return 0;
    }

    if ((PQnfields( result ) != 1) || (PQntuples( result ) != 1))
    {
        debug() << "POSTGRESQL INSERT FAILED: " << PQerrorMessage( m_db ) << "\n" << "FAILED SQL: " << curvalSql << "\n";
        PQclear(result);
        return 0;
    }

    id = QString::fromUtf8(PQgetvalue(result, 0, 0)).toInt();
    PQclear(result);

    return id;
}


void
PostgresqlCollection::setPostgresqlError()
{
    m_error = i18n("Postgresql reported the following error:<br/>") + PQerrorMessage(m_db)
            + i18n("<p>You can configure Postgresql in the Collection section under Settings->Configure Amarok</p>");
}
