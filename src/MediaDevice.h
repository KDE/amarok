// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// (c) 2005 Seb Ruiz <ruiz@kde.org>  
// (c) 2006 T.R.Shashwath <trshash84@gmail.com>
// (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
// See COPYING file for licensing information

#ifndef AMAROK_MEDIADEVICE_H
#define AMAROK_MEDIADEVICE_H

#include "amarok_export.h"
#include "amarok.h"
#include "mediabrowser.h"
#include "meta.h"

#include <QObject>

#include <kio/global.h>      //filesize_t
#include <KUrl>            //stack allocated

class MediaDevice;
class MediaItem;
class TransferDialog;
class MediaView;

/* at least the pure virtual functions have to be implemented by a media device,
   all items are stored in a hierarchy of MediaItems,
   when items are manipulated the MediaItems have to be updated accordingly */

class AMAROK_EXPORT MediaDevice : public QObject, public Amarok::Plugin
{
    Q_OBJECT
    friend class DeviceConfigureDialog;
    friend class MediaBrowser;
    friend class MediaDeviceConfig;
    friend class MediaView;
    friend class MediaQueue;
    friend class TransferDialog;

    public:
        enum Flags
        {
            None = 0,
            OnlyPlayed = 1,
            DeleteTrack = 2,
            Recursing = 4
        };

        MediaDevice();
        virtual void init( MediaBrowser* parent );
        virtual ~MediaDevice();

        MediaView *view();

        /**
         * @return a KAction that will be plugged into the media device browser toolbar
         */
        virtual KAction *customAction() { return 0; }

        virtual void rmbPressed( Q3ListViewItem *item, const QPoint &point, int ) { (void)item; (void) point; }

        /**
         * @return list of filetypes playable on this device
         *  (empty list is interpreted as all types are good)
         */
        virtual QStringList supportedFiletypes() { return QStringList(); }

        /**
         * @param track describes track that should be checked
         * @return true if the device is capable of playing the track referred to by track
         */
        virtual bool isPlayable( const Meta::TrackPtr track );

        /**
         * @param track describes track that should be checked
         * @return true if the track is in the preferred (first in list) format of the device
         */
        virtual bool isPreferredFormat( const Meta::TrackPtr track );

        /**
         * @return true if the device is connected
         */
        virtual bool       isConnected() = 0;

        /**
         * Adds particular tracks to a playlist
         * @param playlist parent playlist for tracks to be added to
         * @param after insert following this item
         * @param items tracks to add to playlist
         */
        virtual void       addToPlaylist(MediaItem *playlist, MediaItem *after, QList<MediaItem*> items) { Q_UNUSED(playlist); Q_UNUSED(after); Q_UNUSED(items); }

        /**
         * Create a new playlist
         * @param name playlist title
         * @param parent parent MediaItem of the new playlist
         * @param items tracks to add to the new playlist
         * @return the newly created playlist
         */
        virtual MediaItem *newPlaylist(const QString &name, MediaItem *parent, QList<MediaItem*> items) { Q_UNUSED(name); Q_UNUSED(parent); Q_UNUSED(items); return 0; }

        /**
         * Move items to a directory
         * @param directory new parent of dropped items
         * @param items tracks to add to the directory
         */
        virtual void      addToDirectory( MediaItem *directory, QList<MediaItem*> items ) { Q_UNUSED(directory); Q_UNUSED(items); }

        /**
         * Create a new directory
         * @param name directory title
         * @param parent parent MediaItem of the new directory
         * @param items tracks to add to the new directory
         * @return the newly created directory
         */
        virtual MediaItem *newDirectory( const QString &name, MediaItem *parent ) { Q_UNUSED(name); Q_UNUSED(parent); return 0; }

        /**
         * Notify device of changed tags
         * @param item item to be updated
         * @param changed meta containing new tags
         * @return the changed MediaItem
         */
        virtual MediaItem *tagsChanged( MediaItem *item, const Meta::TrackPtr changed ) { Q_UNUSED(item); Q_UNUSED(changed); return 0; }

        /**
         * Indicate whether the device has a custom transfer dialog
         * @return whether there is a custom dialog
         */
        virtual bool hasTransferDialog() { return false; }

        /**
         * Run the transfer dialog to be used when Transfer is clicked
         */
        virtual void runTransferDialog() {}

        /**
         * Get the transfer dialog, if any
         * @return the transfer dialog, if any, else NULL;
         */
        virtual TransferDialog *getTransferDialog() { return NULL; }

        /**
         * Can be used to explicitly indicate whether a device needs manual configuration
         * @return whether manual configuration is needed
         */
        virtual bool needsManualConfig() { return true; }

        virtual void addConfigElements( QWidget * /*parent*/ ) {}
        virtual void removeConfigElements( QWidget * /*parent*/ ) {}
        virtual void applyConfig() {}
        virtual void loadConfig();

        QString configString( const QString &name, const QString &defValue = QString() );
        void setConfigString( const QString &name, const QString &value );
        bool configBool( const QString &name, bool defValue=false );
        void setConfigBool( const QString &name, bool value );

        void         setRequireMount( const bool b ) { m_requireMount = b; }
        bool         hasMountPoint() { return m_hasMountPoint; }
        void         setDeviceType( const QString &type ) { m_type = type; }
        QString      type() { return m_type; }
        virtual bool autoConnect() { return false; }
        virtual bool asynchronousTransfer() { return false; }
        bool         isTransferring() { return m_transferring; }
        bool         isDeleting() { return m_deleting; }
        bool         isCanceled() { return m_canceled; }
        void         setCanceled( const bool b ) { m_canceled = b; }

        int          progress() const;
        void         setProgress( const int progress, const int total = -1 /* leave total unchanged by default */ );
        void         hideProgress();


        /**
         * @return a unique identifier that is constant across sessions
         */
        QString udi() const { return m_udi; }

        /**
         * @return the name for the device that should be presented to the user
         */
        QString name() const { return m_name; }

        /**
         * @return the filesystem type
         */
        QString fsType() const { return m_fsType; }

        /**
         * @return the device node
         */
        QString deviceNode() const { return m_deviceNode; }

        /*
         * @return the device mount point (or empty if non-applicable or unknown)
         */
        QString mountPoint() const { return m_mountPoint; }

        QString           getTransferDir() { return m_transferDir; }

        void              setSpacesToUnderscores( bool yesno ) { m_spacesToUnderscores = yesno;
            setConfigBool( "spacesToUnderscores", yesno); }
        bool              getSpacesToUnderscores() { return m_spacesToUnderscores; }

        void              setFirstSort( QString text ) { m_firstSort = text;
            setConfigString( "firstGrouping", text ); }
        void              setSecondSort( QString text ) { m_secondSort = text;
            setConfigString( "secondGrouping", text ); }
        void              setThirdSort( QString text ) { m_thirdSort = text;
            setConfigString( "thirdGrouping", text ); }

        virtual KUrl getProxyUrl( const KUrl& /*url*/) { return KUrl(); }
        virtual void customClicked() { return; }

        //BundleList bundlesToSync( const QString &playlistName, const QString &sql );
        Meta::TrackList tracksToSync( const QString &playlistName, const KUrl &url );
        void preparePlaylistForSync( const QString &playlistName, const Meta::TrackList &list );
        /**
        * @return true if track is on any playlist other than playlistToAvoid
        */
        bool isOnOtherPlaylist( const QString &playlistToAvoid, const Meta::TrackPtr track );
        bool isOnPlaylist( const MediaItem &playlist, const Meta::TrackPtr track );
        bool isInTrackList( const Meta::TrackList &list, const Meta::TrackPtr track );
        bool trackMatch( Meta::TrackPtr track1, Meta::TrackPtr track2 );

    public slots:
        void abortTransfer();
        void transferFiles();
        virtual void renameItem( Q3ListViewItem *item ) {(void)item; }
        virtual void expandItem( Q3ListViewItem *item ) {(void)item; }
        bool connectDevice( bool silent=false );
        bool disconnectDevice( bool postdisconnecthook=true );
        void scheduleDisconnect() { m_scheduledDisconnect = true; }

    protected slots:
        void fileTransferred( KIO::Job *job );
        void fileTransferFinished();

    private:
        int  sysCall(const QString & command);
        int  runPreConnectCommand();
        int  runPostDisconnectCommand();
        QString replaceVariables( const QString &cmd ); // replace %m with mount point and %d with device node

        void setUid( const QString &udi ) { m_udi = udi; }

        /**
         * Find a particular track
         * @param track The meta::track of the requested media item
         * @return The MediaItem of the item if found, otherwise NULL
         * @note This may not be worth implementing for non database driven devices, as it could be slow
         */
        virtual MediaItem *trackExists( Meta::TrackPtr track ) { Q_UNUSED(track); return 0; }

    protected:
        /**
         * Get the capacity and freespace available on the device, in bytes
         * @return true if successful
         */
        virtual bool getCapacity( KIO::filesize_t *total, KIO::filesize_t *available ) { Q_UNUSED(total); Q_UNUSED(available); return false; }

        /**
         * Lock device for exclusive access if possible
         */
        virtual bool lockDevice( bool tryOnly = false ) = 0;

        /**
         * Unlock device
         */
        virtual void unlockDevice() = 0;

        /**
         * Connect to device, and populate m_view with MediaItems
         * @return true if successful
         */
        virtual bool openDevice( bool silent=false ) = 0;

        /**
         * Wrap up any loose ends and close the device
         * @return true if successful
         */
        virtual bool closeDevice() = 0;

        /**
         * Write any pending changes to the device, such as database changes
         */
        virtual void synchronizeDevice() = 0;

        /**
         * Copy a track to the device
         * @param track The Meta::TrackPtr of the item to transfer. Will move the item specified by track->url()
         * @return If successful, the created MediaItem in the media device view, else 0
         */
        virtual MediaItem *copyTrackToDevice(const Meta::TrackPtr track) = 0;

        /**
         * Copy track from device to computer
         * @param item The MediaItem of the track to transfer.
         * @param url The URL to transfer the track to.
         * @return The MediaItem transfered.
         */
        virtual void copyTrackFromDevice(MediaItem *item);

        /**
         * Recursively remove MediaItem from the tracklist and the device
         * @param item MediaItem to remove
         * @param onlyPlayed True if item should be deleted only if it has been played
         * @return -1 on failure, number of files deleted otherwise
         */
        virtual int deleteItemFromDevice( MediaItem *item, int flags=DeleteTrack ) = 0;

        /**
         * Abort the currently active track transfer
         */
        virtual void cancelTransfer() { /* often checking m_cancel is enough */ }

        virtual void updateRootItems();

        virtual bool isSpecialItem( MediaItem *item );

        int deleteFromDevice( MediaItem *item=0, int flags=DeleteTrack );

        void purgeEmptyItems( MediaItem *root=0 );
        void syncStatsFromDevice( MediaItem *root=0 );
        void syncStatsToDevice( MediaItem *root=0 );

        bool kioCopyTrack( const KUrl &src, const KUrl &dst );

        QString     m_name;

        bool        m_hasMountPoint;

        QString     m_preconnectcmd;
        QString     m_postdisconnectcmd;
        bool        m_autoDeletePodcasts;
        bool        m_syncStats;

        bool        m_transcode;
        bool        m_transcodeAlways;
        bool        m_transcodeRemove;

        K3ShellProcess   *sysProc;
        MediaBrowser    *m_parent;
        MediaView       *m_view;
        QString          m_transferDir;
        QString          m_firstSort;
        QString          m_secondSort;
        QString          m_thirdSort;
        QString          m_udi;
        QString          m_deviceNode;
        QString          m_mountPoint;
        QString          m_fsType;
        bool             m_wait;
        bool             m_waitForDeletion;
        bool             m_copyFailed;
        bool             m_requireMount;
        bool             m_canceled;
        bool             m_transferring;
        bool             m_deleting;
        bool             m_deferredDisconnect;
        bool             m_scheduledDisconnect;
        bool             m_runDisconnectHook;
        bool             m_spacesToUnderscores;
        bool             m_transfer;
        bool             m_configure;
        bool             m_customButton;

        QString          m_type;

        // root listview items
        MediaItem *m_playlistItem;
        MediaItem *m_podcastItem;
        // items not on the master playlist and not on the podcast playlist are not visible on the ipod
        MediaItem *m_invisibleItem;
        // items in the database for which the file is missing
        MediaItem *m_staleItem;
        // files without database entry
        MediaItem *m_orphanedItem;

        // stow away all items below m_rootItems when device is not current
        QList<Q3ListViewItem*> m_rootItems;
};


#endif /* AMAROK_MEDIADEVICE_H */
