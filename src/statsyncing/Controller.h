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

#ifndef STATSYNCING_CONTROLLER_H
#define STATSYNCING_CONTROLLER_H

#include "amarok_export.h"
// for CollectionManager::CollectionStatus that cannont be fwd-declared
#include "core-impl/collections/support/CollectionManager.h"

#include <QWeakPointer>

class QTimer;

namespace StatSyncing
{
    class Process;
    class Provider;
    typedef QSet<QSharedPointer<Provider> > ProviderPtrSet;
    class ScrobblingService;
    typedef QExplicitlySharedDataPointer<ScrobblingService> ScrobblingServicePtr;

    /**
     * A singleton class that controls statistics synchronization and related tasks.
     */
    class AMAROK_EXPORT Controller : public QObject
    {
        Q_OBJECT

        public:
            explicit Controller( QObject *parent = 0 );
            ~Controller();

        public slots:
            /**
             * Start the whole synchronization machinery. This call returns quickly,
             * way before the synchronization is finished.
             */
            void synchronize();

            /**
             * Register ScrobblingService with StatSyncing controller. Controller than
             * listens to EngineController and calls scrobble() etc. when user plays
             * tracks. Also allows scrobbling for tracks played on just connected iPods.
             *
             * @param service
             */
            void registerScrobblingService( const ScrobblingServicePtr &service );

            /**
             * Forget about ScrobblingService @param service
             */
            void unregisterScrobblingService( const ScrobblingServicePtr &service );

        private slots:
            /**
             * Wait a few seconds and if no collectionUpdate() signal arrives until then,
             * start synchronization. Otherwise postpone the synchronization for a few
             * seconds.
             */
            void delayedStartSynchronization();
            void slotCollectionAdded( Collections::Collection* collection,
                                      CollectionManager::CollectionStatus status );
            void startNonInteractiveSynchronization();
            void synchronize( int mode );

            void saveSettings( const ProviderPtrSet &checkedProviders,
                               const ProviderPtrSet &unCheckedProviders,
                               qint64 checkedFields );

            void slotTrackFinishedPlaying( const Meta::TrackPtr &track, double playedFraction );
            void scrobble( const Meta::TrackPtr &track, double playedFraction = 1.0,
                           const QDateTime &time = QDateTime() );
            void slotResetLastSubmittedNowPlayingTrack();
            void slotUpdateNowPlayingWithCurrentTrack();

        private:
            Q_DISABLE_COPY( Controller )

            /**
             * Return true if important metadata of both tracks is equal.
             */
            bool tracksVirtuallyEqual( const Meta::TrackPtr &first, const Meta::TrackPtr &second );

            // synchronization-related
            QWeakPointer<Process> m_currentProcess;
            QTimer *m_startSyncingTimer;

            // scrobbling-related
            QList<ScrobblingServicePtr> m_scrobblingServices;
            QTimer *m_updateNowPlayingTimer;
            Meta::TrackPtr m_lastSubmittedNowPlayingTrack;
    };

} // namespace StatSyncing

#endif // STATSYNCING_CONTROLLER_H
