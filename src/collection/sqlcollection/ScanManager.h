/*
 *  Copyright (c) 2003-2005 Mark Kretschmann <kretschmann@kde.org>
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

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
class XmlParseJob;

class ScanManager : public QObject
{
    Q_OBJECT

    public:
        ScanManager( SqlCollection *parent );
        ~ScanManager();

        bool isDirInCollection( QString path );
        bool isFileInCollection( const QString &url );

        void setBlockScan( bool blockScan );

    public slots:
        void startFullScan();
        void startIncrementalScan();

    private slots:
        void slotReadReady();
        void slotFinished();
        void slotError(QProcess::ProcessError error);
        void slotJobDone();
        void restartScanner();

    private:
        QStringList getDirsToScan() const;
        void handleRestart();
        void cleanTables();

    private:
        SqlCollection *m_collection;

        AmarokProcess *m_scanner;

        XmlParseJob *m_parser;

        QHash<QString, QString> m_filesAdded;
        QHash<QString, QString> m_filesDeleted;
        QHash<QString, QString> m_changedUrls;

        int m_restartCount;
        bool m_isIncremental;
        bool m_blockScan;
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
        void setFilesAddedHash( QHash<QString, QString>* hash );
        void setFilesDeletedHash( QHash<QString, QString>* hash );
        void setChangedUrlsHash( QHash<QString, QString>* hash );

        void requestAbort();

    signals:
        void incrementProgress();

    private:
        ScanManager *m_scanManager;
        SqlCollection *m_collection;
        bool m_abortRequested;
        bool m_isIncremental;
        QHash<QString, QString> *m_filesAdded;
        QHash<QString, QString> *m_filesDeleted;
        mutable QHash<QString, QString> *m_changedUrls; 
        QXmlStreamReader m_reader;
        QString m_nextData;
        QWaitCondition m_wait;
        QMutex m_mutex;
        QMutex m_abortMutex;
};

#endif
