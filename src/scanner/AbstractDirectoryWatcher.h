/****************************************************************************************
 * Copyright (c) 2010-2013 Ralf Engels <ralf-engels@gmx.de>                             *
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

#ifndef ABSTRACT_DIRECTORY_WATCHER_H
#define ABSTRACT_DIRECTORY_WATCHER_H

#include "amarok_export.h"
#include "GenericScanManager.h"

#include <QObject>
#include <QSet>
#include <QMutex>
#include <QWaitCondition>

#include <threadweaver/Job.h>

#include <QUrl>

class QTimer;
class KDirWatch;

/** The AbstractDirectoryWatcher is a helper object that watches a set of
    directories for a collection and starts an incremental scan as soon
    as something changes.

    You need to implement the collectionFolders method.

    Use the Watcher like this:
    ThreadWeaver::Weaver::instance()->enqueue( ScanDirectoryWatcherJob( this ) );

    Note: When Amarok is started we wait a minute (so that the scanner does not slow down
    the application startup) and then we do a full incremental scan.
    After that we use KDirWatch to track directory changes.
    KDirWatch will not track changes to symbolic links!

    Note: The watcher needs to be a separate job because the KDirWatcher might need a long
    time adding recursive directories.
    This will prevent the directory adding from blocking the UI.
*/
class AMAROK_EXPORT AbstractDirectoryWatcher : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        AbstractDirectoryWatcher();

        void run();
        void abort();

        /** Pauses the emitting of the scan signal */
        void setBlockScanning( bool block );

    Q_SIGNALS:
        /** Requests the scanner to do an incremental scan.
         *  The incremental scan will check for new files or sub-folders.
         *  @param directory The directory to scan or and empty string if every
         *  collection folder should be checked for changes.
         */
        void requestScan( QList<QUrl> directories, GenericScanManager::ScanType type );

    protected Q_SLOTS:
        void delayTimeout();
        void delayedScan( const QString& path );

    protected:
        virtual QList<QString> collectionFolders() = 0;

        /** Adds the given directory to the list of directories for the next scan.  */
        void addDirToList( const QString &directory );

        QTimer* m_delayedScanTimer;
        KDirWatch *m_watcher;

        /** Mutex for the wait condition */
        QMutex m_mutex;
        QWaitCondition m_waitCondition;

        QMutex m_dirsMutex;
        QSet<QUrl> m_scanDirsRequested;

        bool m_aborted;
        bool m_blocked;
};

#endif
