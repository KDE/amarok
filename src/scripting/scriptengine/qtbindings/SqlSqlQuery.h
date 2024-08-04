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

#ifndef SQLSQLQUERY_H
#define SQLSQLQUERY_H

#include "QtBinding.h"

#include <QObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

namespace QtBindings
{
    namespace Sql
    {
        class SqlQuery : public QObject, public QSqlQuery, public QtBindings::Base<SqlQuery>
        {
            Q_OBJECT
        public:
            Q_INVOKABLE SqlQuery(QSqlResult *r);
            Q_INVOKABLE SqlQuery(const QString& query = QString(), QSqlDatabase db = QSqlDatabase());
            Q_INVOKABLE SqlQuery(QSqlDatabase db);
            Q_INVOKABLE explicit SqlQuery(const QSqlQuery& other);
            Q_INVOKABLE SqlQuery(const SqlQuery& other);
            SqlQuery &operator=(const SqlQuery& other);
        public Q_SLOTS:
            void addBindValue(const QVariant& val, QSql::ParamType type = QSql::In);
            int at() const;
            void bindValue(const QString& placeholder, const QVariant& val, QSql::ParamType type = QSql::In);
            void bindValue(int pos, const QVariant& val, QSql::ParamType type = QSql::In);
            QVariant boundValue(const QString& placeholder) const;
            QVariant boundValue(int pos) const;
            void clear();
            const QSqlDriver* driver() const;
            bool exec();
            bool execBatch(BatchExecutionMode mode = ValuesAsRows);
            bool exec(const QString& query);
            QString executedQuery() const;
            void finish();
            bool first();
            bool isActive() const;
            bool isForwardOnly() const;
            bool isNull(const QString &name) const;
            bool isNull(int field) const;
            bool isSelect() const;
            bool isValid() const;
            bool last();
            QSqlError lastError() const;
            QVariant lastInsertId() const;
            QString lastQuery() const;
            bool next();
            bool nextResult();
            QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;
            int numRowsAffected() const;
            bool prepare(const QString& query);
            bool previous();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QMap<QString, QVariant> boundValues() const;
#else
            QList<QVariant> boundValues() const;
#endif
            QSqlRecord record() const;
            const QSqlResult* result() const;
            bool seek(int i, bool relative = false);
            void setForwardOnly(bool forward);
            void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
            int size() const;
            QVariant value(const QString& name) const;
            QVariant value(int i) const;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Sql::SqlQuery)
#endif //SQLSQLQUERY_H
