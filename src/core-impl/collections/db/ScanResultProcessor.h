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

#ifndef AMAROK_DATABASE_SCANRESULTPROCESSOR_H
#define AMAROK_DATABASE_SCANRESULTPROCESSOR_H

#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "amarok_databasecollection_export.h"

namespace CollectionScanner
{
    class Album;
    class Track;
    class Directory;
    class Playlist;
}

namespace Collections
{
    class DatabaseCollection;
}

class SqlStorage;
class DatabaseUpdater;

/** The ScanResulProcessor class takes the results from the ScanManager and puts them into the database.
 *  Some notes regarding performance:
 *  Scanning of nearly 10000 tracks on my lokal disk takes over 2 minutes.
 *  The second time it only a little over 4 seconds. However I would not see the
 *  second scan as a valid usecase.
 *  Putting 10000 tracks from memory directly into the database takes around
 *  6 seconds. This is what the old ScanResultProcessor did. Writing
 *  big blocks of data, deleting everything else as it goes.
 *  Using normal database access it takes around 12 seconds.
 *  Two times as much but with the additional benefit of a much clearer design.
 *
 *  If one would like to improve this time the following idea would work:
 *  The SqlRegistry reads every url in advance from the database in bulk.
 *  Now all selects for url's are unneeded.
 */
class AMAROK_DATABASECOLLECTION_EXPORT_TESTS ScanResultProcessor : public QObject
{
    Q_OBJECT

    public:
    /** The scan mode.
     *  In general a full scan will consider the information read from the disk
     *  as being superiour to the one in the database.
     *  The full scan will overwrite existing album covers and statistics.
     *
     *  An update scan is a scan done automatically by Amarok. I will check
     *  for new and changed tracks and will add to information already existing
     *  in the database.
     *
     *  A partial update scan is an update scan that does not cover all directories, so
     *  the processor cannot rely on getting all directories from the scanner
     */
        enum ScanType
        {
            FullScan = 0,
            UpdateScan = 1,
            PartialUpdateScan = 2
        };

        ScanResultProcessor( Collections::DatabaseCollection *collection, ScanType type, QObject *parent = 0 );
        virtual ~ScanResultProcessor();

        /** Submits a new directory for processing.
         *  ScanResulProcessor takes ownership of the pointer.
         */
        void addDirectory( CollectionScanner::Directory *dir );

        void commit();
        void rollback();

    protected:
        virtual void commitAlbum( const CollectionScanner::Album *album, int directoryId ) = 0;
        virtual void commitTrack( const CollectionScanner::Track *track, int directoryId, int albumId = -1 ) = 0;

        /** Deletes all directories (and its tracks) not contained in m_foundDirectories */
        virtual void deleteDeletedDirectories() = 0;

        /** Removes all tracks contained in the directory dirId that are not contained in m_foundTracks. */
        virtual void deleteDeletedTracks( int dirId ) = 0;

        Collections::DatabaseCollection *m_collection;

        QList<CollectionScanner::Directory*> m_directories;

        QHash<int, QString> m_foundDirectories;
        QSet<int> m_foundTracks;

        ScanType m_type;

    private:
        void commitDirectory( const CollectionScanner::Directory *dir );
        void commitPlaylist( const CollectionScanner::Playlist *playlist );

};

#endif
