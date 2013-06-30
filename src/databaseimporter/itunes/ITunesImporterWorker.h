/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_ITUNES_IMPORTER_WORKER_H
#define AMAROK_ITUNES_IMPORTER_WORKER_H

#include "databaseimporter/DatabaseImporter.h"
#include "core/meta/forward_declarations.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QMap>
#include <QSqlDatabase>
#include <QXmlStreamReader>

namespace Collections
{
    class CollectionLocation;
}

class ITunesImporterWorker : public ThreadWeaver::Job, public QXmlStreamReader
{
    Q_OBJECT

    public:
        ITunesImporterWorker();
        ~ITunesImporterWorker();

        void setDatabaseLocation( const QString &location ) { m_databaseLocation = location; }
        void setCollectionLocation( Collections::CollectionLocation *location ) { m_collectionLocation = location; }

        bool failed() const { return m_failed; }
        void abort() { m_aborted = true; }

        virtual void run();

    signals:
        void importError( QString );
        void showMessage( QString );
        void trackAdded( Meta::TrackPtr );

    private:
        void readTrackElement();

        bool m_aborted;
        bool m_failed;

        QMap<Meta::TrackPtr, QString> m_tracksForInsert;
        QString m_databaseLocation;
        Collections::CollectionLocation *m_collectionLocation;
};

#endif // multiple inclusion guard
