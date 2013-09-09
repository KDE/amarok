/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#ifndef STATSYNCING_IMPORTER_SQL_TRACK_H
#define STATSYNCING_IMPORTER_SQL_TRACK_H

#include "statsyncing/SimpleWritableTrack.h"
#include <QObject>

#include <QExplicitlySharedDataPointer>
#include <QSqlDatabase>

namespace StatSyncing {

class ImporterSqlProvider;
typedef QExplicitlySharedDataPointer<ImporterSqlProvider> ImporterSqlProviderPtr;

/**
 * A convenience class for SimpleWritableTrack implementations using QtSql framework.
 * It guarantees that QSqlDatabase object is used in the right thread, as per
 * @link http://qt-project.org/doc/qt-4.8/threads-modules.html#threads-and-the-sql-module
 *
 * This class is meant to be used along with ImporterSqlProvider and it uses its
 * non-private methods through friend declaration.
 */
class AMAROK_EXPORT ImporterSqlTrack : public QObject, public SimpleWritableTrack
{
    Q_OBJECT

public:
    ImporterSqlTrack( const ImporterSqlProviderPtr &provider,
                      const Meta::FieldHash &metadata,
                      const QSet<QString> &labels = QSet<QString>() );
    ~ImporterSqlTrack();

protected:
    /**
     * Reimplemented from SimpleWritableTrack. Delegates to sqlCommit ensuring SQL
     * interaction happens in the right thread.
     */
    void doCommit( const QSet<qint64> &fields );

    /**
     * Reimplement this method to save statistics changes to the database.
     * @see SimpleWritableTrack::doCommit() for more details.
     *
     * This method is guaranteed to be called in the main thread. Before it's called,
     * provider's ImporterSqlProvider::prepareConnection() is called.
     * @param db is a result of QSqlDatabase::database( connectionName ), where
     * connectionName is ImporterSqlTrack's construction parameter.
     */
    virtual void sqlCommit( QSqlDatabase db, const QSet<qint64> &fields ) = 0;

private:
    ImporterSqlProviderPtr m_provider;

private slots:
    void slotSqlCommit( const QSet<qint64> &fields );
};

} // namespace StatSyncing

#endif // STATSYNCING_IMPORTER_SQL_TRACK_H
