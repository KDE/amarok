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
    class Config;
    class Process;
    class Provider;
    typedef QExplicitlySharedDataPointer<Provider> ProviderPtr;
    typedef QList<ProviderPtr> ProviderPtrList;
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

            /**
             * Return a list of Meta::val* fields that statistics synchronization can
             * actually synchronize.
             */
            static QList<qint64> availableFields();

            /**
             * Register a StatSyncing::Provider with StatSyncing controller. This makes
             * it possible to synchronize provider with other providers. You don't need
             * to call this for Collections that are registered through CollectionManager
             * (and marked as enabled there) as it is done automatically.
             */
            void registerProvider( const ProviderPtr &provider );

            /**
             * Forget about StatSyncing::Provider @param provider.
             */
            void unregisterProvider( const ProviderPtr &provider );

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

            /**
             * Return StatSyncing configuration object that describes enabled and
             * disabled statsyncing providers. You may not cache the pointer.
             */
            Config *config();

        public slots:
            /**
             * Start the whole synchronization machinery. This call returns quickly,
             * way before the synchronization is finished.
             */
            void synchronize();

            /**
             * Scrobble a track using all registered scrobbling services. They may check
             * certain criteria such as track length and refuse to scrobble the track.
             *
             * @param track track to scrobble
             * @param playedFraction fraction which has been actually played, or a number
             *                       greater than 1 if the track was played multiple times
             *                       (for example on a media device)
             * @param time time when it was played, invalid QDateTime signifies that the
             *             track has been played just now. This is the default when the
             *             parameter is omitted.
             */
            void scrobble( const Meta::TrackPtr &track, double playedFraction = 1.0,
                           const QDateTime &time = QDateTime() );

        signals:
            /**
             * Emitted when a track passed to scrobble() is succesfully queued for
             * scrobbling submission. This signal is emitted for every scrobbling service.
             * For each service, you either get this or scrobbleFailed().
             */
            void trackScrobbled( const ScrobblingServicePtr &service, const Meta::TrackPtr &track );

            /**
             * Emitted when a scrobbling service @service was unable to scrobble() a track.
             *
             * @param error is a ScrobblingService::ScrobbleError enum value.
             */
            void scrobbleFailed( const ScrobblingServicePtr &service, const Meta::TrackPtr &track, int error );

        private slots:
            /**
             * Can only be connected to provider changed() signal
             */
            void slotProviderUpdated();
            /**
             * Wait a few seconds and if no collectionUpdate() signal arrives until then,
             * start synchronization. Otherwise postpone the synchronization for a few
             * seconds.
             */
            void delayedStartSynchronization();
            void slotCollectionAdded( Collections::Collection* collection,
                                      CollectionManager::CollectionStatus status );
            void slotCollectionRemoved( const QString &id );
            void startNonInteractiveSynchronization();
            void synchronize( int mode );

            void slotTrackFinishedPlaying( const Meta::TrackPtr &track, double playedFraction );
            void slotResetLastSubmittedNowPlayingTrack();
            void slotUpdateNowPlayingWithCurrentTrack();

        private:
            Q_DISABLE_COPY( Controller )

            /**
             * Return true if important metadata of both tracks is equal.
             */
            bool tracksVirtuallyEqual( const Meta::TrackPtr &first, const Meta::TrackPtr &second );

            // synchronization-related
            ProviderPtrList m_providers;
            QWeakPointer<Process> m_currentProcess;
            QTimer *m_startSyncingTimer;
            Config *m_config;

            // scrobbling-related
            QList<ScrobblingServicePtr> m_scrobblingServices;
            QTimer *m_updateNowPlayingTimer;
            Meta::TrackPtr m_lastSubmittedNowPlayingTrack;
    };

} // namespace StatSyncing

#endif // STATSYNCING_CONTROLLER_H
