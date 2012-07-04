/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009-2010 Jeff Mitchell <mitchell@kde.org>                             *
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

#include "SqlCollection.h"
#include <core-impl/collections/db/ScanResultProcessor.h>

#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "amarok_sqlcollection_export.h"

namespace Collections
{
    class DatabaseCollection;
}

namespace CollectionScanner
{
    class Album;
    class Track;
    class Directory;
    class Playlist;
}

class SqlStorage;
class DatabaseUpdater;

/** The ScanResulProcessor class takes the results from the ScanManager and puts them into the database.
 */
class AMAROK_SQLCOLLECTION_EXPORT_TESTS SqlScanResultProcessor : public ScanResultProcessor
{
    Q_OBJECT

    public:
        SqlScanResultProcessor( Collections::SqlCollection *collection, QObject *parent = 0 );
        virtual ~SqlScanResultProcessor();

        virtual void commit();

    protected:
        virtual void blockUpdates();
        virtual void unblockUpdates();

        void commitDirectory( CollectionScanner::Directory *dir );
        void commitAlbum( CollectionScanner::Album *album );
        void commitTrack( CollectionScanner::Track *track, CollectionScanner::Album *srcAlbum );

        /** Deletes all directories (and it's tracks) not contained in m_foundDirectories */
        void deleteDeletedDirectories();

        void deleteDeletedTracks( CollectionScanner::Directory *directory );

        /** Removes all tracks contained in the directory dirId that are not contained in m_foundTracks. */
        void deleteDeletedTracks( int directoryId );

    private:
        void removeTrack( int urlId, const QString uid );

        Collections::SqlCollection* m_collection;

        /** Contains all found directories with the directory id and the path */
        QHash<int, CollectionScanner::Directory*> m_foundDirectories;
        /** Contains all found tracks with the unique id */
        QSet<QString> m_foundTracks;

        QHash<CollectionScanner::Directory*, int> m_directoryIds;
        QHash<CollectionScanner::Album*, int> m_albumIds;

        // to speed up the scanning we buffer the whole urls table
        struct UrlEntry
        {
            int id;
            QString path;
            int directoryId;
            QString uid;
        };

        void urlsCacheInit();
        void urlsCacheInsert( UrlEntry entry );
        void urlsCacheRemove( int id );

        /// maps UrlEntry id to UrlEntry
        QHash<int, UrlEntry> m_urlsCache;
        /// maps uid to UrlEntry id
        QHash<QString, int> m_uidCache;
        /// maps path to UrlEntry id
        QHash<QString, int> m_pathCache;
        /// maps directory if to UrlEntry id
        QMultiHash<int, int> m_directoryCache;

};

#endif
