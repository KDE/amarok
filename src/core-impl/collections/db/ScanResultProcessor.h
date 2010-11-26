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

#include <QList>
#include <QHash>
#include <QMultiHash>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>

#include "amarok_databasecollection_export.h"

namespace CollectionScanner
{
    class Track;
    class Directory;
    class Album;
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
            In general a full scan will consider the information read from the disk
            as being superiour to the one in the database.
            The full scan will overwrite existing album covers and statistics.

            An update scan is a scan done automatically by Amarok. I will check
            for new and changed tracks and will add to information already existing
            in the database.

            A partial update scan is an update scan that does not cover all directories, so
            the processor cannot rely on getting all directories from the scanner
         */
        enum ScanType
        {
            FullScan = 0,
            UpdateScan = 1,
            PartialUpdateScan = 2
        };

        // The keys for this hashtable are album name, artist (artist is empty for compilations)
        typedef QPair<QString, QString> AlbumKey;

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

        virtual void commitDirectory( CollectionScanner::Directory *dir );
        virtual void commitPlaylist( CollectionScanner::Playlist *playlist );
        virtual void commitAlbum( CollectionScanner::Album *album ) = 0;
        virtual void commitTrack( CollectionScanner::Track *track, CollectionScanner::Album *album ) = 0;

        /** Deletes all directories (and its tracks) not contained in m_foundDirectories
        */
        virtual void deleteDeletedDirectories() = 0;

        /** Removes all tracks contained in the directory dirId that are not longer present.
        */
        virtual void deleteDeletedTracks( CollectionScanner::Directory *directory) = 0;

        CollectionScanner::Album* sortTrack( CollectionScanner::Track *track );
        CollectionScanner::Album* sortTrack( CollectionScanner::Track *track,
                                             const QString &albumName,
                                             const QString &albumArtist );

        QList<CollectionScanner::Directory*> m_directories;
        QHash<AlbumKey, CollectionScanner::Album*> m_albums;
        // all the albums sorted by album name
        QMultiHash<QString, CollectionScanner::Album*> m_albumNames;

        ScanType m_type;
};

#endif
