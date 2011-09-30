/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2010-2011 Ralf Engels <ralf-engels@gmx.de>                             *
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include <utilities/collectionscanner/Directory.h>

#include <KProcess>
#include <threadweaver/Job.h>
#include <KUrl>

#include <QObject>
#include <QSet>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <QXmlStreamReader>

class DirWatchJob;
class GenericScannerJob;

class QTimer;
class QSharedMemory;

class KDirWatch;

/** The ScanManager manages the scanning and directory watching.

    The scan manager will check the version of the amarokcollectionscanner application,
    watch directories using the KDirWatch and initiate the scanning.

    For the scanning an external process with the scanner is started and the result
    is handled in a seperate thread.
*/
class AMAROK_EXPORT GenericScanManager : public QObject
{
    Q_OBJECT
    public:
        explicit GenericScanManager( QObject *parent = 0 );
        virtual ~GenericScanManager();

        /** Aborts a currently running scan and blocks starting new scans if set to true.
         *  After blockScan has been set to false the scan will not be resumed.
         *  Requested scans will be delayed until the blocking has stopped.
         *  If the scan has been blocked twice it needs to be unblocked twice again.
         */
        virtual void blockScan();

        virtual void unblockScan();

        virtual bool isRunning();

    public slots:
        /** Requests the scanner to do a full scan at the next possibility. */
        virtual void requestFullScan( QList<KUrl> directories );

        /** Abort the request and all currently running scans.
         *  Note: this function does not block further scans, so better call blockScan if you want
         *  to stop scanning for a while.
         */
        virtual void abort( const QString &reason = QString() );

        //TODO: watch directory support

    signals:
        void directoryScanned( CollectionScanner::Directory *dir );

        /** This are status messages that the scanner emits frequently */
        void message( QString message );

        void succeeded();
        void failed( QString message );

    private slots:
        void slotJobDone();

    private:
        GenericScannerJob *m_scannerJob;
        int m_blockCount;

        /**
         * This mutex is protecting the variables:
         * m_scannerJob, m_blockCount
         */
        QMutex m_mutex;
};

/** This is the job that does all the hard work with scanning.
    It will receive new data from the scanner process, parse it and call the
    ScanResultProcessor.
    The job will delete itself when finished or aborted.
*/
class GenericScannerJob : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        /** Returns the path to the collection scanner */
        static QString scannerPath();

        /** Creates the parse job.
            The constructor itself should be called from the UI thread.
            @param xmlFilePath An optional xml file that is parsed
        */
        GenericScannerJob( QObject *parent, QIODevice *input = 0 );

        GenericScannerJob( QObject *parent, QStringList scanDirsRequested, bool recursive = true,
                           bool detectCharset = false );

        ~GenericScannerJob();

        /** Set the batchfile that is used by the scanner to get directory access times */
        void setBatchFile( const QString &batchfilePath ) { m_batchfilePath = batchfilePath; }

        /* ThreadWeaver::Job virtual methods */
        virtual void run();
        virtual void requestAbort();

    // note: since this job doesn't have it's own event queue all signals and slots
    // go through the UI event queue
    Q_SIGNALS:
        void endProgressOperation( QObject * );
        void incrementProgress();
        void totalSteps( int totalSteps );

        void directoryScanned( CollectionScanner::Directory *dir );

        /** This are status messages that the scanner emits frequently */
        void message( QString message );

        void failed( QString message );
        // and the ThreadWeaver::Job also emits done

    private:
        /** Creates the scanner process.
            @returns AmarokProcess pointer
        */
        KProcess *createScannerProcess( bool restart = false );

        void parseScannerOutput();

        QStringList m_scanDirsRequested;
        QIODevice *m_input;

        int m_restartCount;
        bool m_abortRequested;
        QString m_abortReason;
        QString m_incompleteTagBuffer; // strings received via addNewXmlData but not terminated by either a </directory> or a </scanner>

        KProcess *m_scanner;
        QString m_batchfilePath;
        QSharedMemory *m_scannerStateMemory; // a persistent storage of the current scanner state in case it needs to be restarted.
        QString m_sharedMemoryKey;
        bool m_recursive;
        bool m_charsetDetect;

        QXmlStreamReader m_reader;

        QMutex m_mutex; // only protects m_abortRequested and the abort reason
};

#endif // GENERICSCANMANAGER_H
