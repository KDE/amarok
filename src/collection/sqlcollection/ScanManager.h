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

#ifndef AMAROK_SQL_SCANMANAGER_H
#define AMAROK_SQL_SCANMANAGER_H

#include "AmarokProcess.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QWaitCondition>
#include <QXmlStreamReader>

#include <threadweaver/Job.h>

class SqlCollection;
class SqlCollectionDBusHandler;
class SqlStorage;
class XmlParseJob;

class ScanManager : public QObject
{
    Q_OBJECT

    public:
        ScanManager( QObject *parent );
        ~ScanManager();

        bool isDirInCollection( QString path );
        bool isFileInCollection( const QString &url );

        void setBlockScan( bool blockScan );

        //DI setters
        void setCollection( SqlCollection * collection );
        void setStorage( SqlStorage *storage ) { m_storage = storage; }

    public slots:
        void startFullScan();
        void startIncrementalScan( const QString &directory = QString() );
        void abort( const QString &reason = QString() );

    private slots:
        void slotWatchFolders();
        void slotReadReady();
        void slotFinished();
        void slotError(QProcess::ProcessError error);
        void slotJobDone();
        void restartScanner();

    private:
        QStringList getDirsToScan();
        void handleRestart();
        void cleanTables();
        void checkTables( bool full = true );
        void stopParser();
        void writeBatchIncrementalInfoFile();
        bool readBatchFile( QString fileLocation );
        
    private:
        SqlCollection *m_collection;
        SqlCollectionDBusHandler *m_dbusHandler;
        SqlStorage *m_storage;

        AmarokProcess *m_scanner;

        XmlParseJob *m_parser;

        int m_restartCount;
        bool m_isIncremental;
        bool m_blockScan;
        QStringList m_incrementalDirs;
};

class XmlParseJob : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        XmlParseJob( ScanManager *parent, SqlCollection *collection );
        ~XmlParseJob();

        void run();

        void addNewXmlData( const QString &data );

        void setIsIncremental( bool incremental );

        void requestAbort();

    signals:
        void incrementProgress();

    private:
        ScanManager *m_scanManager;
        SqlCollection *m_collection;

        bool m_abortRequested;
        bool m_isIncremental;

        QXmlStreamReader m_reader;
        QString m_nextData;
        QWaitCondition m_wait;

        QMutex m_mutex;
        QMutex m_abortMutex;
};

#endif
