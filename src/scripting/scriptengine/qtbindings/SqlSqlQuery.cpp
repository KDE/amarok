/*
 * Replacement fot QT Bindings that were removed from QT5
 * Copyright (C) 2020  Pedro de Carvalho Gomes <pedrogomes81@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SqlSqlQuery.h"

#include <QVariant>

using namespace QtBindings::Sql;

SqlQuery::SqlQuery(QSqlResult *r) : QSqlQuery(r)
{
}

SqlQuery::SqlQuery(const QString &query, QSqlDatabase db) : QSqlQuery(query, db)
{
}

SqlQuery::SqlQuery(QSqlDatabase db) : QSqlQuery(db)
{
}

SqlQuery::SqlQuery(const QSqlQuery &other) : QSqlQuery(other)
{
}

SqlQuery::SqlQuery(const SqlQuery &other) : QObject(), QSqlQuery(other)
{
}

void SqlQuery::addBindValue(const QVariant &val, QSql::ParamType type)
{
    QSqlQuery::addBindValue(val, type);
}

int SqlQuery::at() const
{
    return QSqlQuery::at();
}

void
SqlQuery::bindValue(const QString &placeholder, const QVariant &val, QSql::ParamType type)
{
    QSqlQuery::bindValue(placeholder, val, type);
}

void SqlQuery::bindValue(int pos, const QVariant &val, QSql::ParamType type)
{
    QSqlQuery::bindValue(pos, val, type);
}

QVariant SqlQuery::boundValue(const QString &placeholder) const
{
    return QSqlQuery::boundValue(placeholder);
}

QVariant SqlQuery::boundValue(int pos) const
{
    return QSqlQuery::boundValue(pos);
}

void SqlQuery::clear()
{
    QSqlQuery::clear();
}

const QSqlDriver *SqlQuery::driver() const
{
    return QSqlQuery::driver();
}

bool SqlQuery::exec()
{
    return QSqlQuery::exec();
}

bool SqlQuery::execBatch(QSqlQuery::BatchExecutionMode mode)
{
    return QSqlQuery::execBatch(mode);
}

bool SqlQuery::exec(const QString &query)
{
    return QSqlQuery::exec(query);
}

QString SqlQuery::executedQuery() const
{
    return QSqlQuery::executedQuery();
}

void SqlQuery::finish()
{
    QSqlQuery::finish();
}

bool SqlQuery::first()
{
    return QSqlQuery::first();
}

bool SqlQuery::isActive() const
{
    return QSqlQuery::isActive();
}

bool SqlQuery::isForwardOnly() const
{
    return QSqlQuery::isForwardOnly();
}

bool SqlQuery::isNull(const QString &name) const
{
    return QSqlQuery::isNull(name);
}

bool SqlQuery::isNull(int field) const
{
    return QSqlQuery::isNull(field);
}

bool SqlQuery::isSelect() const
{
    return QSqlQuery::isSelect();
}

bool SqlQuery::isValid() const
{
    return QSqlQuery::isValid();
}

bool SqlQuery::last()
{
    return QSqlQuery::last();
}

QSqlError SqlQuery::lastError() const
{
    return QSqlQuery::lastError();
}

QVariant SqlQuery::lastInsertId() const
{
    return QSqlQuery::lastInsertId();
}

QString SqlQuery::lastQuery() const
{
    return QSqlQuery::lastQuery();
}

bool SqlQuery::next()
{
    return QSqlQuery::next();
}

bool SqlQuery::nextResult()
{
    return QSqlQuery::nextResult();
}

QSql::NumericalPrecisionPolicy SqlQuery::numericalPrecisionPolicy() const
{
    return QSqlQuery::numericalPrecisionPolicy();
}

int SqlQuery::numRowsAffected() const
{
    return QSqlQuery::numRowsAffected();
}

bool SqlQuery::prepare(const QString &query)
{
    return QSqlQuery::prepare(query);
}

bool SqlQuery::previous()
{
    return QSqlQuery::previous();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QMap<QString, QVariant> SqlQuery::boundValues() const
#else
QList<QVariant> SqlQuery::boundValues() const
#endif
{
    return QSqlQuery::boundValues();
}

QSqlRecord SqlQuery::record() const
{
    return QSqlQuery::record();
}

const QSqlResult *SqlQuery::result() const
{
    return QSqlQuery::result();
}

bool SqlQuery::seek(int i, bool relative)
{
    return QSqlQuery::seek(i, relative);
}

void SqlQuery::setForwardOnly(bool forward)
{
    QSqlQuery::setForwardOnly(forward);
}

void SqlQuery::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
    QSqlQuery::setNumericalPrecisionPolicy(precisionPolicy);
}

int SqlQuery::size() const
{
    return QSqlQuery::size();
}

QVariant SqlQuery::value(const QString &name) const
{
    return QSqlQuery::value(name);
}

QVariant SqlQuery::value(int i) const
{
    return QSqlQuery::value(i);
}

SqlQuery &SqlQuery::operator=(const SqlQuery &other)
{
    if (this != &other) {
        this->QSqlQuery::operator=( other );
    }
    return *this;
}
