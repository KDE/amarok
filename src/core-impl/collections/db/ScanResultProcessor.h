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

#include "core/meta/Meta.h"

#include <QFileInfo>
#include <QList>
#include <QHash>
#include <QMultiHash>
#include <QSet>
#include <QString>
#include <QStringList>

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

        ScanResultProcessor( QObject *parent = 0 );
        virtual ~ScanResultProcessor();

        void setType( ScanType type )
        { m_type = type; }

        /** Submits a new directory for processing.
            ScanResulProcessor takes ownership of the pointer.
        */
        void addDirectory( CollectionScanner::Directory *dir );

        virtual void commit();
        virtual void rollback();

    Q_SIGNALS:
        /** This signal is emitted after a directory was written to the database.
        */
        void directoryCommitted();

    protected:
        /** Will block the collection from noticing the many small updates we are doing */
        virtual void blockUpdates() = 0;
        virtual void unblockUpdates() = 0;

        /** Will get a directory id for a given directory path and modification time.
            If the directory entry is not present, then it will be created.
        */
        virtual int getDirectory( const QString &path, uint mtime ) = 0;

        virtual void commitDirectory( const CollectionScanner::Directory *dir );
        virtual void commitPlaylist( const CollectionScanner::Playlist *playlist );
        virtual void commitAlbum( const CollectionScanner::Album *album, int directoryId ) = 0;
        virtual void commitTrack( const CollectionScanner::Track *track, int directoryId, int albumId = -1 ) = 0;

        /** Deletes all directories (and its tracks) not contained in m_foundDirectories
        */
        virtual void deleteDeletedDirectories() = 0;

        /** Removes all tracks contained in the directory dirId that are not contained in m_foundTracks.
        */
        virtual void deleteDeletedTracks( int dirId ) = 0;

        QList<CollectionScanner::Directory*> m_directories;

        /** Contains all found directories with the directory id and the path */
        QHash<int, QString> m_foundDirectories;
        /** Contains all found tracks with the unique id */
        QSet<QString> m_foundTracks;

        ScanType m_type;
};

#endif
