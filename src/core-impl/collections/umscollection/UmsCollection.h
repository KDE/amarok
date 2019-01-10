/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef UMSCOLLECTION_H
#define UMSCOLLECTION_H

#include "collectionscanner/Directory.h"
#include "core/collections/Collection.h"
#include "core/meta/Observer.h"
#include "core-impl/collections/support/MemoryCollection.h"

#include <KDirWatch>
#include <QIcon>
#include <QDir>
#include <solid/device.h>

#include <QtGlobal>
#include <QTimer>

class GenericScanManager;
class UmsPodcastProvider;
class UmsCollection;
class UmsCollectionLocation;
class QAction;

using namespace Collections;

class UmsCollectionFactory : public CollectionFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-umscollection.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        UmsCollectionFactory();
        virtual ~UmsCollectionFactory();

        void init() override;

    private Q_SLOTS:
        /**
         * Called when solid notifier detects a new device has been added
         */
        void slotAddSolidDevice( const QString &udi );

        /**
         * Called when solid StorageAccess device we are interested in is mounted or
         * unmounted
         */
        void slotAccessibilityChanged( bool accessible, const QString &udi );

        /**
         * Called when solid notifier detects a device has been removed
         */
        void slotRemoveSolidDevice( const QString &udi );

        /**
         * Like @see slotRemoveSolidDevice(), but instructs Collection to eject the
         * device after it has performed necessary teardown operations.
         *
         * Called when user wants to unmount the device from for example Device Notifier
         */
        void slotRemoveAndTeardownSolidDevice( const QString &udi );

        /**
         * Called when "tracked" collection is destroyed
         */
        void slotCollectionDestroyed( QObject *collection );

    private:
        /**
         * Checks whether a solid device is a USB mass-storage one
         */
        bool identifySolidDevice( const QString &udi ) const;

        /**
         * Attempts to create appropriate collection for already identified solid device
         * @param udi. Should Q_EMIT newCollection() if the collection was successfully
         * created and should become visible to the user.
         */
        void createCollectionForSolidDevice( const QString &udi );

        // maps device udi to active UMS collections
        QMap<QString, UmsCollection *> m_collectionMap;
};

class UmsCollection : public Collection, public Meta::Observer
{
    Q_OBJECT

    public:
        // inherited methods

        explicit UmsCollection( const Solid::Device &device );
        virtual ~UmsCollection();

        /* TrackProvider methods */
        bool possiblyContainsTrack( const QUrl &url ) const override;
        Meta::TrackPtr trackForUrl( const QUrl &url ) override;

        /* Collection methods */
        QueryMaker *queryMaker() override;
        QString uidUrlProtocol() const override;

        QString collectionId() const override;
        QString prettyName() const override;
        QIcon icon() const override;

        bool hasCapacity() const override;
        float usedCapacity() const override;
        float totalCapacity() const override;

        CollectionLocation *location() override;

        bool isOrganizable() const override;

        /* Capability-related methods */
        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability *createCapabilityInterface(
                Capabilities::Capability::Type type ) override;

        /* Meta::Observer methods */
        void metadataChanged( const Meta::TrackPtr &track ) override;
        using Meta::Observer::metadataChanged; // silence compiler warning about hidder overloads

        /* own methods */
        const QUrl &musicUrl() const { return m_musicUrl; }
        const QUrl &podcastUrl() const { return m_podcastUrl; }

        /**
         * Get location where track @param track should be transferred to.
         *
         * @param fileExtension new extension to use. Leave empty if you don't wish to
         * change file extension
         */
        QUrl organizedUrl( const Meta::TrackPtr &track, const QString &fileExtension = QString() ) const;

        QSharedPointer<MemoryCollection> memoryCollection() const { return m_mc; }

    Q_SIGNALS:
        /**
         * Start a count-down that emits updated() signal after it expires.
         * Resets the timer to original timeout if already running. This is to ensure
         * that we Q_EMIT update() max. once per \<timeout\> for batch updates.
         *
         * Timers can only be started from "their" thread so use signals & slots for that.
         */
        void startUpdateTimer();

    public Q_SLOTS:
        /**
         * Destroy the collection (by emitting remove)
         */
        void slotDestroy();

        /**
         * Destroy the collection and try to eject the device from system
         */
        void slotEject();

        void slotTrackAdded( const QUrl &trackLocation );
        void slotTrackRemoved( const Meta::TrackPtr &track );

    private Q_SLOTS:
        /**
         * Update m_lastUpdated timestamp and Q_EMIT updated()
         */
        void collectionUpdated();

        void slotParseTracks();
        void slotParseActionTriggered();
        void slotConfigure();

        void slotDirectoryScanned( QSharedPointer<CollectionScanner::Directory> dir );

        /**
         * Starts a timer that ensures we Q_EMIT updated() signal sometime in future.
         */
        void slotStartUpdateTimer();

    private:
        /** extended constructor */
        void init();

        //static variables relating to the on-disk configuration file
        static QString s_settingsFileName;
        static QString s_musicFolderKey;
        static QString s_musicFilenameSchemeKey;
        static QString s_vfatSafeKey;
        static QString s_asciiOnlyKey;
        static QString s_postfixTheKey;
        static QString s_replaceSpacesKey;
        static QString s_regexTextKey;
        static QString s_replaceTextKey;
        static QString s_podcastFolderKey;
        static QString s_autoConnectKey;
        static QString s_collectionName;
        static QString s_transcodingGroup;

        Solid::Device m_device;
        QSharedPointer<MemoryCollection> m_mc;
        bool m_tracksParsed;

        bool m_autoConnect;
        QString m_mountPoint;
        QUrl m_musicUrl;
        QUrl m_podcastUrl;
        QString m_musicFilenameScheme;
        bool m_vfatSafe;
        bool m_asciiOnly;
        bool m_postfixThe;
        bool m_replaceSpaces;
        QString m_regexText;
        QString m_replaceText;
        QString m_collectionName;
        QString m_collectionId;

        GenericScanManager *m_scanManager;
        KDirWatch m_watcher;

        QStringList m_supportedMimeTypes;

        UmsPodcastProvider *m_podcastProvider;

        QAction *m_parseAction;
        QAction *m_configureAction;
        QAction *m_ejectAction;
        QTimer m_updateTimer;
        qint64 m_lastUpdated; /* msecs since epoch */
};

#endif
