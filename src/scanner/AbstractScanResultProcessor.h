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

#ifndef ABSTRACT_SCAN_RESULTPROCESSOR_H
#define ABSTRACT_SCAN_RESULTPROCESSOR_H

#include "amarok_export.h"
#include "GenericScanManager.h"

#include <QList>
#include <QHash>
#include <QMultiHash>
#include <QPair>
#include <QString>

namespace CollectionScanner
{
    class Track;
    class Directory;
    class Album;
    class Playlist;
}
class GenericScanManager;
template<class T >
class QSharedPointer;

/** The ScanResulProcessor class connects to a GenericScanManager and
 *  adds further processing to the directoryScanned signal.
 *
 *  The AbstractScanResultProcessor can determine if tracks need to be removed
 *  and better sort Tracks into albums.
 */
class AMAROK_EXPORT AbstractScanResultProcessor : public QObject
{
    Q_OBJECT

    public:
        explicit AbstractScanResultProcessor( GenericScanManager* manager, QObject* parent = nullptr );
        ~AbstractScanResultProcessor() override;

    Q_SIGNALS:
        /** Those signals can be connected to a progress bar to get progress information
         *  from the scanner.
         */
        void endProgressOperation( QObject * );
        void incrementProgress();
        void totalSteps( int totalSteps );

    protected Q_SLOTS:
        // Signals received from the ScanManager.
        // They must be send in the correct order:
        // first scanStarted
        // then scandirectoryCount, scanDirectoryScanned in no particular order number
        // then scanSucceeded or scanFailed.

        virtual void scanStarted( GenericScanManager::ScanType type );

        virtual void scanDirectoryCount( int count );

        /** Submits a new directory for processing. */
        virtual void scanDirectoryScanned( QSharedPointer<CollectionScanner::Directory> dir );

        virtual void scanSucceeded();
        virtual void scanFailed( const QString& message );

        virtual void abort();

    protected:
        /** This are status messages that the scanner emits frequently.
         *  You can collect them and display later.
         */
        virtual void message( const QString& message ) = 0;

        virtual void commitDirectory( QSharedPointer<CollectionScanner::Directory> dir );
        virtual void commitPlaylist( const CollectionScanner::Playlist &playlist );
        virtual void commitAlbum( CollectionScanner::Album *album ) = 0;
        virtual void commitTrack( CollectionScanner::Track *track, CollectionScanner::Album *srcAlbum ) = 0;

        /**
         * Delete directories (and their tracks) that have been deleted.
         *
         * In FullScan nad UpdateScan mode, it should delete all database directories on
         * mounted filesystems not encountered in the scan.
         *
         * In PartialUpdateScan mode, things are more complicated. It should only delete
         * not-encountered (direct) subdirectories of directories passed to
         * deleteDeletedTracksAndSubdirs(). However, if such directory is deleted, all its
         * subdirectories should be removed too.
         */
        virtual void deleteDeletedDirectories() = 0;

        virtual void deleteDeletedTracksAndSubdirs( QSharedPointer<CollectionScanner::Directory> directory ) = 0;

        CollectionScanner::Album* sortTrack( CollectionScanner::Track *track,
                                             const QString &dirName = QString() );

        /** Cleans up all the stuff that the processor picked up through
         *  the commit methods.
         *  This function is called by scanSucceeded and scanFailed.
         */
        virtual void cleanupMembers();

        GenericScanManager* m_manager;

        QList<QSharedPointer<CollectionScanner::Directory> > m_directories;

        // The keys for this hashtable are album name, artist (artist is empty for compilations)
        typedef QPair<QString, QString> AlbumKey;

        QHash<AlbumKey, CollectionScanner::Album*> m_albums;
        // all the albums sorted by album name
        QMultiHash<QString, CollectionScanner::Album*> m_albumNames;

        GenericScanManager::ScanType m_type;
};

#endif
