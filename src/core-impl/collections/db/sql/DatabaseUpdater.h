/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_DATABASEUPDATER_H
#define AMAROK_DATABASEUPDATER_H

#include <QHash>
#include <QString>

#include "amarok_sqlcollection_export.h"


namespace Collections {
    class SqlCollection;
}

/** The DatabaseUpdater class is a collection of function that can update the sql database from previous versions or create it from new.
 */
class AMAROK_SQLCOLLECTION_EXPORT DatabaseUpdater {
public:
    explicit DatabaseUpdater( Collections::SqlCollection *collection );
    ~DatabaseUpdater();

    static int expectedDatabaseVersion();

    /**
     * Return true if the current database schema is outdated or non-existent and needs to
     * be updated using to update() method.
     */
    bool needsUpdate() const;

    /**
     * Return true if database schema already exists in the database. When this method
     * returns false, needsUpdate() returns true.
     */
    bool schemaExists() const;

    /**
     * Updates the database to @see expectedDatabaseVersion()
     * @returns true if a update was performed, false otherwise
     */
    bool update();

    void upgradeVersion1to2();
    void upgradeVersion2to3();
    void upgradeVersion3to4();
    void upgradeVersion4to5();
    void upgradeVersion5to6();
    void upgradeVersion6to7();
    void upgradeVersion7to8();
    void upgradeVersion8to9();
    void upgradeVersion9to10();
    void upgradeVersion10to11();
    void upgradeVersion11to12();
    void upgradeVersion12to13();
    void upgradeVersion13to14();
    void upgradeVersion14to15();
    void upgradeVersion15to16();

    /** Checks the given table for redundant entries.
     *  Table can be artist,album,genre,composer,urls or year
     *  Note that you should check album table first so that the rest can
     *  be deleted fully.
     *  Note: it can handle also the directories table but that is better
     *  be left to the ScannerProcessor
     *
     *  TODO: also check the remaining 8 tables
     */
    void deleteAllRedundant( const QString &type );

    /**
     * Delete all entries from @p table with broken link to directories table. Currently
     * had only sense on the urls table.
     */
    void deleteOrphanedByDirectory( const QString &table );

    /**
     * Use on tables that link to non-existent entries in the urls table, for example
     * urls_labels, statistics, lyrics. @param table must have a column named url.
     */
    void deleteOrphanedByUrl( const QString &table );

    void removeFilesInDir( int deviceid, const QString &rdir );

    /** Cleans up the database
     *  The current functionality is quite fuzzy because it does not have anything.
     *  to clean.
     */
    void cleanupDatabase();

    /** Checks all tables for errors.
     *  It does not try to repair as this could be fatal.
     *  The check results should be in the log file so the user would send them
     *  to the developers upon request.
     */
    void checkTables( bool full = true );

    void writeCSVFile( const QString &table, const QString &filename, bool forceDebug = false );

    static int textColumnLength() { return 255; }

private:
    /** creates all the necessary tables, indexes etc. for the database */
    void createTables() const;

    int adminValue( const QString &key ) const;

    Collections::SqlCollection *m_collection;
    bool m_debugDatabaseContent;
};

#endif
