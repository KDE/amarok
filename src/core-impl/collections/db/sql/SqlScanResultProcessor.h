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
        virtual int getDirectory( const QString &path, uint mtime );

        void commitAlbum( const CollectionScanner::Album *album, int directoryId );
        void commitTrack( const CollectionScanner::Track *track, int directoryId, int albumId = -1 );

        /** Deletes all directories (and it's tracks) not contained in m_foundDirectories */
        void deleteDeletedDirectories();

        /** Removes all tracks contained in the directory dirId that are not contained in m_foundTracks. */
        void deleteDeletedTracks( int dirId );

    private:
        void removeTrack( int urlId, const QString uid );

        Collections::SqlCollection* m_collection;

        // to speed up the scanning we buffer the whole urls table
        struct UrlEntry
        {
            int id;
            QString path;
            int directoryId;
            QString uid;
        };

        void cacheUrlsInit();
        void cacheUrlsInsert( const UrlEntry &entry );
        void cacheUrlsRemove( int id );

        QHash<int, UrlEntry> m_urlsCache;
        QHash<QString, int> m_urlsCacheUid;
        QHash<QString, int> m_urlsCachePath;
        QMultiHash<int, int> m_urlsCacheDirectory;

};

#endif
