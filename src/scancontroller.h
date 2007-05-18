/***************************************************************************
 *   Copyright (C) 2003-2007 by The Amarok Developers                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef AMAROK_SCANCONTROLLER_H
#define AMAROK_SCANCONTROLLER_H

#include <QEvent>
#include <QMutex>
#include <QtXml>         //baseclass

#include "threadmanager.h" //baseclass

class CollectionDB;
class K3ProcIO;

/**
 * @class ScanController
 * @short Starts and controls the external amarokcollectionscanner application.
 * @author Mark Kretschmann <markey@web.de>
 *
 * The collection scanner itself is run in an external process, unlike before, where it
 * used to be thread. The advantage is that the scanner cannot crash the Amarok main
 * application any more. If it crashes we can simply restart it.
 *
 * Amarok communicates with the scanner via the ScanController class, which processes
 * XML entities written to stdout by the scanner process. For XML parsing an event
 * driven SAX2 parser is used, which can process the entities as they arrive, without
 * the need for a DOM document structure.
 */

class ScanController : public ThreadManager::DependentJob, public QXmlDefaultHandler
{
    Q_OBJECT

    public:
        static const int RestartEventType = 8891;

        class RestartEvent : public QEvent {
            public:
                RestartEvent() : QEvent( uniqueType() ) {}
                static Type uniqueType() { return Type( RestartEventType ); }
        };

        static const int PlaylistFoundEventType = 8890;

        class PlaylistFoundEvent : public QEvent {
            public:
                PlaylistFoundEvent( QString path )
                    : QEvent( uniqueType() )
                    , m_path( path ) {}
                static Type uniqueType() { return Type( PlaylistFoundEventType ); }
                QString path() { return m_path; }
            private:
                QString m_path;
        };

    public:
        ScanController( CollectionDB* parent, bool incremental, const QStringList& folders = QStringList() );
        ~ScanController();
        static ScanController* instance();

        virtual void completeJob( void );

        bool isIncremental() const { return m_incremental; }
        bool hasChanged() const { return m_hasChanged; }

        bool tablesCreated() { return m_tablesCreated; }

    signals:
        void scannerAcknowledged();
        void scanDone( bool changed );

    public slots:
        void slotFileMoved( const QString &src, const QString &dest );

    private slots:
        void slotReadReady();

    private:
        void initIncremental();
        virtual bool doJob();
        static void setInstance( ScanController* instance );

        bool startElement( const QString&, const QString &localName, const QString&, const QXmlAttributes &attrs );
        void customEvent( QEvent* );

        // Member variables:
        static const int MAX_RESTARTS = 80;
        static const int MAX_FAILURE_PERCENTAGE = 5;

        K3ProcIO* m_scanner;
        QStringList m_folders;
        QStringList m_foldersToRemove;
        bool m_incremental;
        bool m_hasChanged;

        QString m_xmlData;
        QMutex m_dataMutex;
        QXmlInputSource* m_source;
        QXmlSimpleReader* m_reader;

        QStringList m_crashedFiles;

        // Every file that the collection scanner finds is marked
        // here, as well as the source of all files that the AFT code
        // detects as having been moved.  These are the files that
        // have definitely not been deleted.  The key is the absolute
        // path.
        QMap<QString,QString> m_filesAdded;
        QMap<QString,QString> m_filesDeleted;
        QMutex             m_fileMapsMutex;

        static ScanController* currController;

        bool m_tablesCreated;
        int m_scanCount;
};


#endif // AMAROK_SCANCONTROLLER_H
