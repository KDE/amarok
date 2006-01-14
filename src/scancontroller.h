/***************************************************************************
 *   Copyright (C) 2003-2005 by The amaroK Developers                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROK_SCANCONTROLLER_H
#define AMAROK_SCANCONTROLLER_H

#include <qobject.h>      //baseclass
#include <qxml.h>         //baseclass

#include "threadweaver.h" //baseclass

class CollectionDB;
class KProcIO;

/**
 * @class ScanController
 * @short Starts and controls the external amarokcollectionscanner application.
 * @author Mark Kretschmann <markey@web.de>
 *
 * The collection scanner itself is run in an external process, unlike before, where it
 * used to be thread. The advantage is that the scanner cannot crash the amaroK main
 * application any more. If it crashes we can simply restart it.
 *
 * amaroK communicates with the scanner via the ScanController class, which processes
 * XML entities written to stdout by the scanner process. For XML parsing an event
 * driven SAX2 parser is used, which can process the entities as they arrive, without
 * the need for a DOM document structure.
 */
class ScanController : public QXmlDefaultHandler, public ThreadWeaver::DependentJob
{
    public:
        static const int PlaylistFoundEventType = 8890;

        class PlaylistFoundEvent : public QCustomEvent {
        public:
            PlaylistFoundEvent( QString path )
                : QCustomEvent( PlaylistFoundEventType )
                , m_path( path ) {}
            QString path() { return m_path; }
        private:
            QString m_path;
        };

    public:
        ScanController( CollectionDB* parent, const QStringList& folders = QStringList() );
        ~ScanController();

    protected:
        virtual bool doJob();

        // Member variables:
        KProcIO* const m_scanner;
        QStringList m_folders;
        QStringList m_foldersToRemove;

        bool m_incremental;
        bool m_scannerCrashed;

    private:
        bool startElement( const QString&, const QString &localName, const QString&, const QXmlAttributes &attrs );

        // Member variables:
        QXmlInputSource m_source;
        QXmlSimpleReader m_reader;
};


/**
 * @class IncrementalScanController
 * @short Only scans directories that have been modified since the last scan
 */

class IncrementalScanController : public ScanController
{
    public:
        IncrementalScanController( CollectionDB* );

        bool hasChanged() const { return m_hasChanged; }

    protected:
        bool doJob();

        bool m_hasChanged;
};


#endif // AMAROK_SCANCONTROLLER_H

