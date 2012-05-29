/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef STATSYNCING_PROCESS_H
#define STATSYNCING_PROCESS_H

#include "statsyncing/Provider.h"

#include <QMap>

namespace ThreadWeaver {
    class Job;
}
class QAbstractItemModel;

namespace StatSyncing
{
    class MatchedTracksPage;

    /**
     * Class that is responsible for one synchronization process from track matching
     * to commiting synchronized values back to storage. This class should live in a main
     * thread and is event-based.
     *
     * Process auto-deletes itself when it is done with its work.
     */
    class Process : public QObject
    {
        Q_OBJECT

        public:
            /**
             * Creates the synchronization process that will offer user to synchronize
             * statistics of @param providers. Process does _not_ take ownership of the
             * provider pointers.
             */
            explicit Process( const QList<QSharedPointer<Provider> > &providers,
                              QObject *parent = 0 );
            ~Process();

        public slots:
            /**
             * Starts the process.
             */
            void start();

            /**
             * Raises and activates possible UI window related to this synchronization
             * process.
             */
            void raise();

        private slots:
            void slotTracksMatched( ThreadWeaver::Job* job );

            void showMatchedTracks( bool really );

            void showUniqueTracks( bool checked );
            void showExcludedTracks( bool checked );
            /**
             * Helper method for show{Unique,Excluded}Tracks
             */
            void showSingleTracks( const QMap<const Provider *, QAbstractItemModel *> &models );

            void changeUniqueTracksProvider( int index );
            void changeExcludedTracksProvider( int index );
            /**
             * Helper method for change{UniqueExcluded}TracksProvider
             */
            void changeSingleTracksProvider( int index, const QMap<const Provider *, QAbstractItemModel *> &models );

        private:
            Q_DISABLE_COPY( Process )

            QList<QSharedPointer<Provider> > m_providers;

            MatchedTracksPage *m_matchedTracksPage;
            QAbstractItemModel *m_matchedTracksModel;
            QMap<const Provider *, QAbstractItemModel *> m_uniqueTracksModels;
            QMap<const Provider *, QAbstractItemModel *> m_excludedTracksModels;
    };

} // namespace StatSyncing

// needed for QCombobox payloads:
Q_DECLARE_METATYPE( const StatSyncing::Provider * )

#endif // STATSYNCING_PROCESS_H
