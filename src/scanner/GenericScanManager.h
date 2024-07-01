/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2010-2011 Ralf Engels <ralf-engels@gmx.de>                             *
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef GENERICSCANMANAGER_H
#define GENERICSCANMANAGER_H

#include "amarok_export.h"
#include "collectionscanner/Directory.h"

#include <QObject>
#include <QMutex>
#include <QSharedPointer>
#include <QString>
#include <QUrl>

class GenericScannerJob;

/**
 * The ScanManager manages the scanning and directory watching.
 *
 * The scan manager will check the version of the amarokcollectionscanner application,
 * watch directories using the KDirWatch and initiate the scanning.
 *
 * For the scanning an external process with the scanner is started and the result
 * is handled in a separate thread.
 */
class AMAROK_EXPORT GenericScanManager : public QObject
{
    Q_OBJECT
    public:
        explicit GenericScanManager( QObject* parent = nullptr );
        ~GenericScanManager() override;

        /** The scan mode.
            In general a full scan will consider the information read from the disk
            as being superior to the one in the database.
            The full scan will overwrite existing album covers and statistics.

            An update scan is a scan done automatically by Amarok. I will check
            for new and changed tracks and will add to information already existing
            in the database.

            A partial update scan is an update scan that does not cover all directories, so
            the processor cannot rely on getting all directories from the scanner

            TODO: the ScanResultProcessor should be smart enough to figure out if directories
            should be removed.
         */
        enum ScanType
        {
            /** This type of scan is done from the configuration dialog.
             *  It should react as if it's the first time that the collection is scanned.
             *  So it might e.g. throw away album covers added by the user.
             *
             *  The ScanResultProcessor may assume that directories not found
             *  during the scan are really gone (and not just excluded).
             */
            FullScan = 0,

            /** This scan type scans the whole set of directories from a collection.
             *  The ScanResultProcessor may assume that directories not found
             *  during the scan are really gone (and not just excluded).
             */
            UpdateScan = 1,

            /** This is an UpdateScan that does not include the whole set from a collection.
             *  That means that directories not reported by the ScannerJob might not
             *  have been excluded.
             */
            PartialUpdateScan = 2
        };

        /** Returns true if the scanner job is currently scanning */
        virtual bool isRunning();

        /** Write the batch file
         *  The batch file contains the known modification dates so that the scanner only
         *  needs to report changed directories
         *  @returns the path to the batch file or an empty string if nothing was written.
         */
        virtual QString getBatchFile( const QStringList& scanDirsRequested );


    public Q_SLOTS:
        /** Requests the scanner to do a full scan at the next possibility. */
        virtual void requestScan( QList<QUrl> directories, GenericScanManager::ScanType type = UpdateScan );

        /** Requests the scanner to do a full scan using the given import file.
         */
        virtual void requestImport( QIODevice *input, GenericScanManager::ScanType type = UpdateScan );

        /** Abort the request and all currently running scans.
         *  Note: this function does not block further scans, so better
         *  stop your directory watcher to stop scanning for a while.
         */
        virtual void abort();

    Q_SIGNALS:
        // the following signals are created by the scanner job and just
        // routed through
        // They are directly connected to the GenericScannerJob, so
        // beware of multi-threading

        void started( GenericScanManager::ScanType type );

        /** Gives the estimated count of directories that this scan will have.
            This signal might not be emitted or emitted multiple times if the
            count is updated.
        */
        void directoryCount( int count );

        /** Emitted once we get the complete data for a directory.
         *  @param dir The directory structure with all containing tracks.
         *
         *  The dir pointer will stay valid until after the done signal.
         *  Be careful, you need to have direct connections to
         *  ensure that you don't access the pointer before it's being freed.
         *  That also means that your slots are called within the job context.
        */
        void directoryScanned( QSharedPointer<CollectionScanner::Directory> dir );

        void succeeded();
        void failed( const QString& message );

    protected:
        /** Connects all the signals to m_scannerJob */
        void connectSignalsToJob();

        QWeakPointer<GenericScannerJob> m_scannerJob;

        /**
         * This mutex is protecting the variables:
         * m_scannerJob
         */
        QMutex m_mutex;
};

Q_DECLARE_METATYPE(GenericScanManager::ScanType);

#endif // GENERICSCANMANAGER_H
