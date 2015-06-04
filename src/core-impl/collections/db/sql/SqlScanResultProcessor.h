/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009-2010 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_SQL_SCANRESULTPROCESSOR_H
#define AMAROK_SQL_SCANRESULTPROCESSOR_H

#include "scanner/AbstractScanResultProcessor.h"
#include "core-impl/collections/db/sql/SqlCollection.h"

/** The ScanResulProcessor class takes the results from the ScanManager and puts them into the database.
 */
class SqlScanResultProcessor : public AbstractScanResultProcessor
{
    Q_OBJECT

    public:
        SqlScanResultProcessor( GenericScanManager* manager,
                                Collections::SqlCollection *collection,
                                QObject *parent = 0 );
        virtual ~SqlScanResultProcessor();


    protected Q_SLOTS:
        virtual void scanStarted( GenericScanManager::ScanType type );
        virtual void scanSucceeded();

        virtual void displayMessages();

    protected:
        virtual void message( const QString& message );

        virtual void commitDirectory( QSharedPointer<CollectionScanner::Directory> directory );
        virtual void commitAlbum( CollectionScanner::Album *album );
        virtual void commitTrack( CollectionScanner::Track *track, CollectionScanner::Album *srcAlbum );

        /** Deletes all directories (and it's tracks) not contained in m_foundDirectories */
        virtual void deleteDeletedDirectories();

        virtual void deleteDeletedTracksAndSubdirs( QSharedPointer<CollectionScanner::Directory> directory );

        /** Removes all tracks contained in the directory dirId that are not contained in m_foundTracks. */
        virtual void deleteDeletedTracks( int directoryId );

        virtual void cleanupMembers();

        void blockUpdates();
        void unblockUpdates();

    private:
        // to speed up the scanning we buffer the whole urls table
        struct UrlEntry {
            int id;
            QString path;
            int directoryId;
            QString uid;
        };
        friend QDebug operator<<( QDebug, const UrlEntry& );

        /**
         * Finds best url id of a track identified by @param uid uniqueid in caches. If
         * multiple entries are found, tries to use @param path as a hint.
         *
         * @returns url id or -1 if nothing is found.
         */
        int findBestUrlId( const QString &uid, const QString &path );

        /**
         * Directory id has changed from @param oldDirId to @param newDirId without
         * actual change in the absolute directory path or contents. Try to relocate
         * tracks to the new directory, updating necessary fields.
         * @return true if all tracks were successfully relocated and the old dir can be
         * deleted without losses, false otherwise.
         */
        bool relocateTracksToNewDirectory( int oldDirId, int newDirId );

        /**
         * Remove track from the database. Does not touch the cache - you should probably
         * call urlsCacheRemove( entry )
         */
        void removeTrack( const UrlEntry &entry );

        struct DirectoryEntry {
            int dirId;
            int deviceId;
            QString dir;
        };

        /**
         * Get a list of all mounted directories from the database.
         */
        QList<DirectoryEntry> mountedDirectories() const;

        /**
         * Get a list of directories that have been physically removed during the
         * PartialUpdateScan.
         */
        QList<DirectoryEntry> deletedDirectories() const;

        Collections::SqlCollection *m_collection;

        /**
         * Contains all found directories with their absolute path and id
         */
        QHash<QString, int> m_foundDirectories;

        /**
         * Contains all found tracks with the unique id and url id. QMultiHash only
         * because it implements contains( key, value )
         */
        QMultiHash<QString, int> m_foundTracks;

        /**
         * In UpdateScan and PartialUpdateScan this set gets filled with directory ids
         * that have been (non-recursively) fully scanned (not skipped). Direct child
         * directories from the database that are not contained in m_foundDirectories can
         * be considered deleted.
         */
        QSet<int> m_scannedDirectoryIds;

        // never dereference they key, it might be a stale pointer in corner cases
        QHash<CollectionScanner::Directory*, int> m_directoryIds;
        QHash<CollectionScanner::Album*, int> m_albumIds;

        void urlsCacheInit();
        /**
         * Inserts @param entry into caches. If an entry with same url id already exists
         * in cache, it will be replaced. Duplicates in uidUrl and path are _not_ avoided,
         * m_uidCache and m_pathCache will point to most recently added entry.
         */
        void urlsCacheInsert( const UrlEntry &entry );
        void urlsCacheRemove( const UrlEntry &entry );

        /// maps UrlEntry id to UrlEntry
        QHash<int, UrlEntry> m_urlsCache;
        /// maps uid to UrlEntry id
        QMultiHash<QString, int> m_uidCache;
        /// maps path to UrlEntry id
        QHash<QString, int> m_pathCache;
        /// maps directory id to UrlEntry id
        QMultiHash<int, int> m_directoryCache;

        QDateTime m_blockedTime;
        QStringList m_messages;
};

QDebug operator<<( QDebug dbg, const SqlScanResultProcessor::UrlEntry &entry );

#endif
