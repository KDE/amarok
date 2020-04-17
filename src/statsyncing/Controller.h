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
// for CollectionManager::CollectionStatus that cannot be fwd-declared
#include "core-impl/collections/support/CollectionManager.h"

#include <QPointer>
#include <QDateTime>
#include <QMap>

#include <KLocalizedString>

class QTimer;

namespace StatSyncing
{
    class Config;
    class CreateProviderDialog;
    class Process;
    class Provider;
    typedef QSharedPointer<Provider> ProviderPtr;
    typedef QList<ProviderPtr> ProviderPtrList;
    class ProviderFactory;
    class ScrobblingService;
    typedef QSharedPointer<ScrobblingService> ScrobblingServicePtr;

    /**
     * A singleton class that controls statistics synchronization and related tasks.
     */
    class AMAROK_EXPORT Controller : public QObject
    {
        Q_OBJECT

        public:
            explicit Controller( QObject *parent = nullptr );
            ~Controller() override;

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
            virtual void registerProvider( const ProviderPtr &provider );

            /**
             * Forget about StatSyncing::Provider @p provider.
             * @param provider the provider
             */
            virtual void unregisterProvider( const ProviderPtr &provider );

            /**
             * Handle plugin factories derived from ProviderFactory, used for creating
             * multiple provider instances. This method is called by Amarok's plugin
             * infrastructure.
             */
            void setFactories( const QList<QSharedPointer<Plugins::PluginFactory> > &factories );

            /**
             * Returns true if any instantiatable provider types are registered with the
             * controller.
             */
            bool hasProviderFactories() const;

            /**
             * Returns true if the provider identified by @param id is configurable
             */
            bool providerIsConfigurable( const QString &id ) const;

            /**
             * Returns a configuration dialog for a provider identified by @param id .
             * @returns 0 if there's no provider identified by id or the provider is not
             * configurable, otherwise a pointer to the dialog constructed as a child of
             * The::mainWindow
             */
            QWidget *providerConfigDialog( const QString &id ) const;

            /**
             * Returns a provider creation dialog, prepopulated with registered provider
             * types.
             * @returns a pointer to the dialog constructed as a child of The::mainWindow,
             * and is a subclass of KAssistantDialog.
             *
             * @see StatSyncing::CreateProviderDialog
             */
            QWidget *providerCreationDialog() const;

            /**
             * Register ScrobblingService with StatSyncing controller. Controller then
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
             * Return a list of currently registered scrobbling services (in arbitrary
             * order).
             */
            QList<ScrobblingServicePtr> scrobblingServices() const;

            /**
             * Return StatSyncing configuration object that describes enabled and
             * disabled statsyncing providers. You may not cache the pointer.
             */
            Config *config();

        public Q_SLOTS:
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

        Q_SIGNALS:
            /**
             * Emitted when a track passed to scrobble() is successfully queued for
             * scrobbling submission. This signal is emitted for every scrobbling service.
             * For each service, you either get this or scrobbleFailed().
             */
            void trackScrobbled( const ScrobblingServicePtr &service, const Meta::TrackPtr &track );

            /**
             * Emitted when a scrobbling service @p service was unable to scrobble() a track.
             *
             * @param service the service
             * @param track the track
             * @param error is a ScrobblingService::ScrobbleError enum value.
             */
            void scrobbleFailed( const ScrobblingServicePtr &service, const Meta::TrackPtr &track, int error );

        private Q_SLOTS:
            /**
             * Creates new instance of provider type identified by @param type
             * with configuration stored in @param config.
             */
            void createProvider( const QString &type, const QVariantMap &config );

            /**
             * Reconfigures provider identified by @param id with configuration
             * stored in @param config.
             */
            void reconfigureProvider( const QString &id, const QVariantMap &config );

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
            void synchronizeWithMode( int mode );

            void slotTrackFinishedPlaying( const Meta::TrackPtr &track, double playedFraction );
            void slotResetLastSubmittedNowPlayingTrack();
            void slotUpdateNowPlayingWithCurrentTrack();

        private:
            Q_DISABLE_COPY( Controller )

            ProviderPtr findRegisteredProvider( const QString &id ) const;

            /**
             * Return true if important metadata of both tracks is equal.
             */
            bool tracksVirtuallyEqual( const Meta::TrackPtr &first, const Meta::TrackPtr &second );
            QMap<QString, QSharedPointer<ProviderFactory> > m_providerFactories;

            // synchronization-related
            ProviderPtrList m_providers;
            QPointer<Process> m_currentProcess;
            QTimer *m_startSyncingTimer;
            Config *m_config;

            /**
             * When a new collection appears, StatSyncing::Controller will automatically
             * trigger synchronization. It however waits s_syncingTriggerTimeout
             * milliseconds to let the collection settle down. Moreover, if the newly
             * added collection emits updated(), the timeout will run from start again.
             *
             * (reason: e.g. iPod Collection appears quickly, but with no tracks, which
             * are added gradually as they are parsed. This "ensures" we only start
             * syncing as soon as all tracks are parsed.)
             */
            static const int s_syncingTriggerTimeout;

            // scrobbling-related
            QList<ScrobblingServicePtr> m_scrobblingServices;
            QTimer *m_updateNowPlayingTimer;
            Meta::TrackPtr m_lastSubmittedNowPlayingTrack;
    };

} // namespace StatSyncing

#endif // STATSYNCING_CONTROLLER_H
