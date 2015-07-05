/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef OPMLWRITER_H
#define OPMLWRITER_H

#include "OpmlOutline.h"

#include <ThreadWeaver/Job>
#include <QUrl>

#include <QXmlStreamWriter>

class AMAROK_EXPORT OpmlWriter : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        /** OpmlWriter will write the OPML outline objects as XML text.
          * @arg rootOutlines the <body> of the OPML
          * @arg headerData these fields are put in the <head> of the OPML
          * @arg device QIODevice to write to
          * The children of IncludeNodes will not be written. Remove the type="include" attribute
          * from the include node to force a save of those child nodes.
          */
        OpmlWriter( const QList<OpmlOutline *> rootOutlines,
                    const QMap<QString,QString> headerData,
                    QIODevice *device );

        void setHeaderData( const QMap<QString,QString> data ) { m_headerData = data; }
        /**
         * The function that starts the actual work. Inherited from ThreadWeaver::Job
         * Note the work is performed in a separate thread
         * @return Returns true on success and false on failure
         */
        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) Q_DECL_OVERRIDE;

        QIODevice *device() { return m_xmlWriter->device(); }

    Q_SIGNALS:
        /**
         * Signal emmited when writing is complete.
         */
        void result( int error );

        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

    private:
        void writeOutline( const OpmlOutline *outline );
        QList<OpmlOutline *> m_rootOutlines;
        QMap<QString,QString> m_headerData;

        QUrl m_fileUrl;
        QXmlStreamWriter *m_xmlWriter;

    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) Q_DECL_OVERRIDE;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) Q_DECL_OVERRIDE;

};

#endif // OPMLWRITER_H
