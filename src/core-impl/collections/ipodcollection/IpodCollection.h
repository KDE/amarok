/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz                                       *
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

#ifndef IPODCOLLECTION_H
#define IPODCOLLECTION_H

#include "ui_IpodConfiguration.h"
#include "core/collections/Collection.h"
#include "core/meta/Observer.h"

#include <QMutex>
#include <QSharedPointer>
#include <QTimer>

namespace Collections { class MemoryCollection; }
namespace IpodMeta { class Track; }
class IphoneMountPoint;
class IpodParseTracksJob;
class IpodWriteDatabaseJob;
class IpodPlaylistProvider;
class QDir;
class QTemporaryFile;
struct _Itdb_iTunesDB;
typedef _Itdb_iTunesDB Itdb_iTunesDB;

class IpodCollection : public Collections::Collection, public Meta::Observer
{
    Q_OBJECT

    public:
        /**
         * Creates an iPod collection on top of already-mounted filesystem.
         *
         * @param mountPoint actual iPod mount point to use, must be already mounted and
         * accessible. When eject is requested, solid StorageAccess with this mount point
         * is searched for to perform unmounting.
         * @param uuid filesystem volume UUID or another unique identifier for this iPod
         */
        explicit IpodCollection( const QDir &mountPoint, const QString &uuid );

        /**
         * Creates an iPod collection on top of not-mounted iPhone/iPad by accessing it
         * using libimobiledevice by its 40-digit device UUID. UUID may be empty which
         * means "any connected iPhone/iPad".
         */
        explicit IpodCollection( const QString &uuid );

        virtual ~IpodCollection();

        // TrackProvider methods:
        virtual bool possiblyContainsTrack( const QUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const QUrl &url );

        // CollectionBase methods:
        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        // Collection methods:
        virtual Collections::QueryMaker *queryMaker();

        virtual QString uidUrlProtocol() const;
        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual QIcon icon() const;

        virtual bool hasCapacity() const;
        virtual float usedCapacity() const;
        virtual float totalCapacity() const;

        virtual Collections::CollectionLocation *location();
        virtual bool isWritable() const;
        virtual bool isOrganizable() const;

        // Observer methods:
        virtual void metadataChanged( Meta::TrackPtr track );
        // so that the compiler doesn't complain about hidden virtual functions:
        using Meta::Observer::metadataChanged;

        // IpodCollection methods:
        /**
         * In-fact second phase of the constructor. Called by CollectionFactory right after
         * constructor. Should return true if the collection initialised itself successfully
         * and should be shown to the user; return value of false means it should be
         * destroyed and forgotten by the factory.
         */
        bool init();

        /**
         * Get local mount point. Can return QString() in case no reasonable mountpoint
         * is available
         */
        QString mountPoint();

        /**
         * Return number of bytes that should be kept free in iPod for database operations.
         * CollectionLocation should try hard not to occupy this safety margin.
         */
        float capacityMargin() const;

        /**
         * Return a list of file formats (compatible with Meta::Track::type()) current iPod
         * is able to play.
         */
        QStringList supportedFormats() const;

        /**
         * Return pointer to playlist provider associated with this iPod. May be null in
         * special cases (iPod not yet initialised etc.)
         */
        Playlists::UserPlaylistProvider *playlistProvider() const;

        Meta::TrackPtr trackForUidUrl( const QString &uidUrl );

    Q_SIGNALS:
        /**
         * Start a count-down that emits updated() signal after it expires.
         * Resets the timer to original timeout if already running. This is to ensure
         * that we emit update() max. once per \<timeout\> for batch updates.
         *
         * Timers can only be started from "their" thread so use signals & slots for that.
         */
        void startUpdateTimer();

        /**
         * Start a count-down that initiates iTunes database writing after it expires.
         * Resets the timer to original timeout if already running. This is to ensure
         * that we don't write the database all the time for batch updates.
         *
         * Timers can only be started from "their" thread so use signals & slots for that.
         */
        void startWriteDatabaseTimer();

    public Q_SLOTS:
        /**
         * Destroy the collection, try to write back iTunes database (if dirty)
         */
        void slotDestroy();

        /**
         * Destroy the collection, write back iTunes db (if dirty) and try to eject the
         * iPod from system
         */
        void slotEject();

        /**
         * Shows the configuration dialog in a non-modal window. If m_itdb is null, shows
         * some info and a button to try to initialize iPod.
         */
        void slotShowConfigureDialog();

        /**
         * Shows the configuration dialog in a non-modal window. If m_itdb is null, shows
         * some info and a button to try to initialize iPod.
         */
        void slotShowConfigureDialogWithError( const QString &errorMessage );

    private Q_SLOTS:
        /**
         * Update m_lastUpdated timestamp and emit updated()
         */
        void collectionUpdated();

        /**
         * Tries to initialize iPod, read the database, add tracks. (Re)shows the
         * configuration dialog with info about initialization.
         */
        void slotInitialize();

        /**
         * Sets iPod name to the name in configure dialog.
         */
        void slotApplyConfiguration();

        /**
         * Starts a timer that ensures we emit updated() signal sometime in future.
         */
        void slotStartUpdateTimer();

        /**
         * Starts a timer that initiates iTunes database writing after 30 seconds.
         */
        void slotStartWriteDatabaseTimer();

        /**
         * Enqueues a job in a thread that writes iTunes database back to iPod. Should
         * only be called from m_writeDatabaseTimer's timeout() signal. (with exception
         * when IpodCollection is about to destroy itself)
         */
        void slotInitiateDatabaseWrite();

        /**
         * Tries to unmount underlying solid device. You must try to write database before
         * calling this. Emits remove() before returning.
         */
        void slotPerformTeardownAndRemove();

        /**
         * Do sanity checks and emit remove() so that this collection is destroyed by
         * CollectionManager. No other method is allowed to emit remove()!
         */
        void slotRemove();

    private:
        friend class IpodCopyTracksJob;
        friend class IpodDeleteTracksJob;
        friend class IpodParseTracksJob;
        friend class IpodWriteDatabaseJob;
        friend class IpodPlaylistProvider;

        static const QString s_uidUrlProtocol;
        static const QStringList s_audioFileTypes;
        static const QStringList s_videoFileTypes;
        static const QStringList s_audioVideoFileTypes;

        // method for IpodParseTracksJob and IpodCopyTracksJob:
        /**
         * Add an iPod track to the collection.
         *
         * This method adds it to the collection, master playlist (if not already there)
         * etc. The file must be already physically copied to iPod. (Re)Sets track's
         * collection to this collection. Takes ownership of the track (passes it to
         * AmarokSharedPointer)
         *
         * This method is thread-safe.
         *
         * @return pointer to newly added track if successful, null pointer otherwise
         */
        Meta::TrackPtr addTrack( IpodMeta::Track *track );

        // method for IpodDeleteTracksJob:
        /**
         * Removes a track from iPod collection. Does not delete the file physically,
         * caller must do it after calling this method.
         *
         * @param track a track from associated MemoryCollection to delete. Accepts also
         * underlying IpodMeta::Track, this is treated as if MemoryMeta::Track track
         * proxy it was passed.
         *
         * This method is thread-safe.
         */
        void removeTrack( const Meta::TrackPtr &track );

        // method for IpodWriteDatabaseJob and destructor:
        /**
         * Calls itdb_write() directly. Logs a message about success/failure in Amarok
         * interface.
         */
        bool writeDatabase();

        QDialog *m_configureDialog;
        Ui::IpodConfiguration m_configureDialogUi;
        QSharedPointer<Collections::MemoryCollection> m_mc;
        /**
         * pointer to libgpod iTunes database. If null, this collection is invalid
         * (not yet initialised). Can only be changed with m_itdbMutex hold.
         */
        Itdb_iTunesDB *m_itdb;
        QMutex m_itdbMutex;
        QTimer m_updateTimer;
        qint64 m_lastUpdated; /* msecs since epoch */
        QTimer m_writeDatabaseTimer;
        QTemporaryFile *m_preventUnmountTempFile;
        QString m_mountPoint;
        QString m_uuid;
        IphoneMountPoint *m_iphoneAutoMountpoint;
        QString m_prettyName;
        IpodPlaylistProvider *m_playlistProvider;
        QAction *m_configureAction;
        QAction *m_ejectAction;
        QAction *m_consolidateAction;
        QPointer<IpodParseTracksJob> m_parseTracksJob;
        QPointer<IpodWriteDatabaseJob> m_writeDatabaseJob;
};

#endif // IPODCOLLECTION_H
