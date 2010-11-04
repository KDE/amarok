/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
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

#ifndef AMAROK_DATABASE_SCANMANAGER_H
#define AMAROK_DATABASE_SCANMANAGER_H

#include "AmarokProcess.h"
#include "amarok_databasecollection_export.h"
#include "ScanResultProcessor.h"
#include "DatabaseCollection.h"

#include <QObject>
#include <QProcess>
#include <QSet>
#include <QMutex>
#include <QWaitCondition>
#include <QXmlStreamReader>

#include <threadweaver/Job.h>

class XmlParseJob;

class AMAROK_DATABASECOLLECTION_EXPORT_TESTS ScanManager : public QObject
{
    Q_OBJECT

    public:
        ScanManager( Collections::DatabaseCollection *collection, QObject *parent = 0 );
        virtual ~ScanManager();

        /** Aborts a currently running scan and blocks starting new scans if set to true.
         *  After blockScan has been set to false the scan will be resumed.
         *  requested scans will be delayed until the blocking has stopped.
         *  If the scan has been blocked twice it needs to be unblocked twice again.
         */
        virtual void blockScan();

        virtual void unblockScan();

    public slots:
        /** Requests the scanner to do a full scan at the next possibility.
         *  The full scan will clear the whole database and re-create all data.
         *  Statistics are not touched.
         */
        virtual void requestFullScan();

        /** Requests the scanner to do an incremental scan.
         *  The incremental scan will check for new files or sub-folders.
         *  @param directory The directory to scan or and empty string if every
         *  collection folder should be checked for changes.
         */
        virtual void requestIncrementalScan( const QString &directory = QString() );

        /** Abort the request and all currently running scans. */
        virtual void abort( const QString &reason = QString() );

    private slots:
        /** This slot is called once to check the scanner version.
            An error message is displayed if the versions don't match.
        */
        void slotCheckScannerVersion();

        /** Slot is called when the check folder timer runs out. */
        void slotWatchFolders();

        /** Called when the scanner process has outputted some data. */
        void slotReadReady();

        /** Called when the scanner process is finished. */
        void slotFinished(int exitCode, QProcess::ExitStatus exitStatus);

        /** Called when the scanner process has an error. */
        void slotError(QProcess::ProcessError error);

        /** Called when the parser job has finished. */
        void slotJobDone();

        /* Tries to start the scanner */
        void startScanner();
        void startScannerProcess( bool restart );

    private:

        /** Returns the path to the collection scanner */
        QString scannerPath() const;

        /**
           Returns a list of all directories and their modification time from the
           database.
         */
        QList< QPair<QString, uint> > getKnownDirs();

        void deleteRemovedDirectories();

        void handleRestart();

        void stopParser();

    private:
        Collections::DatabaseCollection *m_collection;

        AmarokProcess *m_scanner;
        XmlParseJob *m_parser;

        int m_restartCount;
        int m_blockCount;

        bool m_fullScanRequested;
        QSet<QString> m_scanDirsRequested;

        bool m_fullScan;
        QSet<QString> m_scanDirs;

        QString m_batchfilePath;

        /**
           This mutex is protecting the variables:
           m_fullScanRequested, m_fullScan, m_scanDirsRequested, m_scanDirs
          */
        QMutex m_mutex;
};

/** This is the job that does all the hard work with scanning.
    It will receive new data from the scanner process, parse it and call the
    ScanResultProcessor.
    The job will delete itself when finished or aborted.
*/
class XmlParseJob : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        /** Creates the parse job.
            The constructor itself should be called from the UI thread.
        */
        XmlParseJob( QObject *parent, Collections::DatabaseCollection *collection,
                     ScanResultProcessor::ScanType scanType );
        ~XmlParseJob();

        void run();
        void addNewXmlData( const QString &data );
        void requestAbort();

    signals:
        void totalSteps( const QObject *o, int totalSteps );
        void step( const QObject *o );

    private:
        Collections::DatabaseCollection *m_collection;
        ScanResultProcessor::ScanType m_scanType;

        bool m_abortRequested;
        QString m_incompleteTagBuffer; // strings received via addNewXmlData but not terminated by either a </directory> or a </scanner>

        QXmlStreamReader m_reader;

        QWaitCondition m_wait;
        QMutex m_mutex;
};

#endif
