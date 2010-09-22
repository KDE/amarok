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

#include <threadweaver/Job.h>
#include <KUrl>

#include <QXmlStreamWriter>

class OpmlWriter : public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        OpmlWriter( const OpmlOutline *rootOutline, QIODevice *device );

        /**
         * The function that starts the actual work. Inherited from ThreadWeaver::Job
         * Note the work is performed in a separate thread
         * @return Returns true on success and false on failure
         */
        void run();

        QIODevice *device() { return m_xmlWriter->device(); }

    signals:
        /**
         * Signal emmited when writing is complete.
         */
        void result( int error );

    private:
        void writeOutline( const OpmlOutline *outline );
        const OpmlOutline *m_rootOutline;
        KUrl m_fileUrl;
        QXmlStreamWriter *m_xmlWriter;
};

#endif // OPMLWRITER_H
