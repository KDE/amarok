/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef AMAROK_SQLCOLLECTIONLOCATION_H
#define AMAROK_SQLCOLLECTIONLOCATION_H

#include "core/collections/CollectionLocation.h"
#include <KJob>
#include <KCompositeJob>
#include <QSet>
#include <QMap>
#include <QString>

namespace Collections {
    class SqlCollection;
}

class SqlCollectionLocation;
/**
 * @class TransferJob
 * A simple class that provides KJob functionality (progress reporting, aborting, etc) for sqlcollectionlocation.
 * It calls SqlCollectionLocation::startNextJob() 
 */
class TransferJob : public KCompositeJob
{
    Q_OBJECT
    public:
        TransferJob( SqlCollectionLocation * location );

        void start();
    virtual bool addSubjob( KJob* job );

    void emitInfo( const QString &message );

    protected slots:
        void doWork();

        /**
         * A move or copy job finished
         */
        void slotJobFinished( KJob *job );
    protected:
        virtual bool doKill();
    private:
        SqlCollectionLocation* m_location;
        bool m_killed;
};

class SqlCollectionLocation : public CollectionLocation
{
    Q_OBJECT

    public:
        SqlCollectionLocation( Collections::SqlCollection const *collection );
        virtual ~SqlCollectionLocation();

        virtual QString prettyLocation() const;
        virtual QStringList actualLocation() const;
        virtual bool isWritable() const;
        virtual bool isOrganizable() const;

        /**
         * Removes a track from the database ONLY if the file does NOT exist on disk.
         * Do not call this method directly. Use the prepareRemove() method.
         * @param track a track that does not exist on disk to be removed from the database
         * @return true if the database entry was removed
         */
        virtual bool remove( const Meta::TrackPtr &track );
        virtual void insertTracks( const QMap<Meta::TrackPtr, QString> &trackMap );
        virtual void insertStatistics( const QMap<Meta::TrackPtr, QString> &trackMap );

    protected:
        virtual void showDestinationDialog( const Meta::TrackList &tracks, bool removeSources );
        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources );
        virtual void removeUrlsFromCollection( const Meta::TrackList &sources );

    private slots:
        void slotDialogAccepted();
        void slotDialogRejected();
        void slotJobFinished( KJob *job );
        void slotRemoveJobFinished( KJob *job );
        void slotTransferJobFinished( KJob *job );
        void slotTransferJobAborted();

    private:
        bool startNextJob();
        bool startNextRemoveJob();

        QMap<QString, uint> updatedMtime( const QStringList &urls );

        Collections::SqlCollection *m_collection;
        QMap<Meta::TrackPtr, QString> m_destinations;
        QMap<Meta::TrackPtr, KUrl> m_sources;
        Meta::TrackList m_removetracks;
        QHash<Meta::TrackPtr, KUrl> m_originalUrls;
        bool m_overwriteFiles;
        QMap<KJob*, Meta::TrackPtr> m_jobs;
        QMap<KJob*, Meta::TrackPtr> m_removejobs;
        TransferJob* m_transferjob;
        friend class TransferJob; // so the transfer job can run the jobs
};

class SqlCollectionLocationFactory
{
    public:
        virtual SqlCollectionLocation* createSqlCollectionLocation() const = 0;
        virtual ~SqlCollectionLocationFactory() {};
};

#endif
