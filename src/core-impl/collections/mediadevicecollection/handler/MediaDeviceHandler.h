/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MEDIADEVICEHANDLER_H
#define MEDIADEVICEHANDLER_H

#include "core/meta/Observer.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceMeta.h"
#include "core-impl/collections/mediadevicecollection/handler/MediaDeviceHandlerCapability.h"
#include "core-impl/collections/mediadevicecollection/handler/capabilities/PlaylistCapability.h"
#include "core-impl/collections/mediadevicecollection/handler/capabilities/PodcastCapability.h"
#include "core-impl/collections/mediadevicecollection/handler/capabilities/ReadCapability.h"
#include "core-impl/collections/mediadevicecollection/handler/capabilities/WriteCapability.h"
#include "core-impl/collections/mediadevicecollection/playlist/MediaDevicePlaylist.h"
#include "core-impl/collections/mediadevicecollection/playlist/MediaDeviceUserPlaylistProvider.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"

#include <ThreadWeaver/Job>

#include <QAction>
#include <QObject>
#include <QMap>
#include <QMultiMap>

class QString;
class QMutex;

namespace Collections {
    class MediaDeviceCollection;
}

namespace Meta
{
    typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

    class MEDIADEVICECOLLECTION_EXPORT MetaHandlerCapability
    {
    public:
        virtual ~MetaHandlerCapability() {}

        virtual bool hasCapabilityInterface( Handler::Capability::Type type ) const;

        virtual Handler::Capability* createCapabilityInterface( Handler::Capability::Type type );

        /**
             * Retrieves a specialized interface which represents a capability of this
             * object.
             *
             * @returns a pointer to the capability interface if it exists, 0 otherwise
             */
        template <class CapIface> CapIface *create()
        {
            Handler::Capability::Type type = CapIface::capabilityInterfaceType();
            Handler::Capability *iface = createCapabilityInterface(type);
            return qobject_cast<CapIface *>(iface);
        }

        /**
             * Tests if an object provides a given capability interface.
             *
             * @returns true if the interface is available, false otherwise
             */
        template <class CapIface> bool is() const
        {
            return hasCapabilityInterface( CapIface::capabilityInterfaceType() );
        }
    };

/**
The MediaDeviceHandler is the backend where all low-level library calls are made.
It exists to leave a generic API in the other classes, while allowing for low-level
calls to be isolated here.
*/

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceHandler : public QObject, public Meta::MetaHandlerCapability, public Meta::Observer
{
    Q_OBJECT

public:
    /**
    * Destructor
    */
    virtual ~MediaDeviceHandler();

    // Declare thread as friend class

    friend class ParseWorkerThread;

    /**
    * Begins an attempt to connect to the device, and emits
    * attemptConnectionDone when it finishes.
    */
    virtual void init() = 0; // collection

    /**
    * Checks if the handler successfully connected
    * to the device.
    * @return
    *   TRUE if the device was successfully connected to
    *   FALSE if the device was not successfully connected to
    */
    bool succeeded() const // collection
    {
        return m_success;
    }

    /// Methods provided for CollectionLocation

    /**
    * Checks if a device can be written to.
    * @return
    *   TRUE if the device can be written to
    *   FALSE if the device can not be written to
    */
    virtual bool isWritable() const = 0;

    /** Given a list of tracks, get URLs for device tracks
    *  of this type of device.  If the device needs to
    *  do some work to get URLs (e.g. copy tracks to a
    *  temporary location) the overridden method in
    *  the handler takes care of it, but must emit
    *  gotCopyableUrls when finished.
    *  @param tracks The list of tracks for which to fetch urls
    */
    virtual void getCopyableUrls( const Meta::TrackList &tracks );

    /**
    * Fetches the human-readable name of the device.
    * This is often called from the Collection since
    * a library call is needed to get this name.
    * @return A QString with the name
    */
    virtual QString prettyName() const = 0;

    /**
    * Copies a list of tracks to the device.
    * @param tracklist The list of tracks to copy.
    */
    void copyTrackListToDevice( const Meta::TrackList tracklist );

    /**
    * Removes a list of tracks from the device.
    * @param tracks The list of tracks to remove.
    */
    void removeTrackListFromDevice( const Meta::TrackList &tracks );

    /** This function is called just before a track in the playlist is to be played, and gives
    *  a chance for e.g. MTP to copy the track to a temporary location, set a playable url,
    *  to emulate the track actually being played off the device
    *  @param track The track that needs to prepare to be played
    */
    virtual void prepareToPlay( Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) } // called by @param track

    virtual float usedcapacity();
    virtual float totalcapacity();

    Playlists::UserPlaylistProvider* provider();

    // HACK: Used for device-specific actions, such as initialize for iPod
    virtual QList<QAction *> collectionActions() { return QList<QAction*> (); }

Q_SIGNALS:
    void gotCopyableUrls( const QMap<Meta::TrackPtr, QUrl> &urls );
    void databaseWritten( bool succeeded );

    void deleteTracksDone();
    void incrementProgress();
    void endProgressOperation( QObject *owner );

    void copyTracksDone( bool success );
    void removeTracksDone();

    /* File I/O Methods */

public Q_SLOTS:

   /**
    * Parses the media device's database and creates a Meta::MediaDeviceTrack
    * for each track in the database.  NOTE: only call once per device.
    */
    void parseTracks(); // collection

   /**
    * Writes to the device's database if it has one, otherwise
    * simply calls slotDatabaseWritten to continue the workflow.
    */

    virtual void writeDatabase() { slotDatabaseWritten( true ); }

    void savePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist, const QString& name );
    void renamePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist );
    void deletePlaylists( const Playlists::MediaDevicePlaylistList &playlistlist );

    bool privateParseTracks();

    void copyNextTrackToDevice();
    bool privateCopyTrackToDevice( const Meta::TrackPtr& track );

    void removeNextTrackFromDevice();
    void privateRemoveTrackFromDevice( const Meta::TrackPtr &track );

    void slotCopyNextTrackFailed( ThreadWeaver::JobPointer job, const Meta::TrackPtr& track );
    void slotCopyNextTrackDone( ThreadWeaver::JobPointer job, const Meta::TrackPtr& track );

protected:

    /**
     * Constructor
     * @param parent the Collection whose handler this is
     */
    MediaDeviceHandler( QObject *parent );

    /**
     * Creates a MediaDeviceTrack based on the latest track struct created as a
     *  result of a copy to the device, and adds it into the collection to reflect
     *  that it has been copied.
     *  @param track The track to add to the collection
     */
    void addMediaDeviceTrackToCollection( Meta::MediaDeviceTrackPtr &track );

    /**
     * Removes the @param track from all the collection's maps to reflect that
     * it has been removed from the collection
     * @param track The track to remove from the collection
     */
    void removeMediaDeviceTrackFromCollection( Meta::MediaDeviceTrackPtr &track );

    /**
     * Uses wrapped libGet methods to fill a track with information from device
     * @param track The track from whose associated struct to get the information
     * @param destTrack The track that we want to fill with information
     */
    void getBasicMediaDeviceTrackInfo( const Meta::MediaDeviceTrackPtr& track, Meta::MediaDeviceTrackPtr destTrack );

    /**
     * Uses wrapped libSet methods to fill a track struct of the particular library
     * with information from a Meta::Track
     * @param srcTrack The track that has the source information
     * @param destTrack The track whose associated struct we want to fill with information
     */
    void setBasicMediaDeviceTrackInfo( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr destTrack );

    Collections::MediaDeviceCollection   *m_memColl; ///< Associated collection

    bool m_success;
    bool m_copyingthreadsafe; ///< whether or not the handler's method of copying is threadsafe
    TitleMap          m_titlemap; ///< Map of track titles to tracks, used to detect duplicates

protected Q_SLOTS:

    void slotFinalizeTrackCopy( const Meta::TrackPtr & track );
    void slotCopyTrackFailed( const Meta::TrackPtr & track );
    void slotFinalizeTrackRemove( const Meta::TrackPtr & track );

    void slotDatabaseWritten( bool success );

    void enqueueNextCopyThread();

    void slotDeletingHandler();

private:

    /**
    * Pulls out meta information (e.g. artist string)
    * from track struct, inserts into appropriate map
    * (e.g. ArtistMap).  Sets track's meta info
    * (e.g. artist string) to that extracted from
    * track struct's.
    * @param track - track being written to
    * @param Map - map where meta information is
    * associated to appropriate meta pointer
    * (e.g. QString artist, ArtistPtr )
    */

    void setupArtistMap( Meta::MediaDeviceTrackPtr track, ArtistMap &artistMap );
    void setupAlbumMap( Meta::MediaDeviceTrackPtr track, AlbumMap &albumMap, ArtistMap &artistMap );
    void setupGenreMap( Meta::MediaDeviceTrackPtr track, GenreMap &genreMap );
    void setupComposerMap( Meta::MediaDeviceTrackPtr track, ComposerMap &composerMap );
    void setupYearMap( Meta::MediaDeviceTrackPtr track, YearMap &yearMap );

    // Misc. Helper Methods

    /**
     * Tries to create read capability in m_rc
     * @return true if m_rc is valid read capability, false otherwise
     */
    bool setupReadCapability();

    /**
     * Tries to create write capability in m_rc
     * @return true if m_wc is valid write capability, false otherwise
     */
    bool setupWriteCapability();

    /**
     *  @return free space on the device
     */
    float freeSpace();

    // Observer Methods

    /** These methods are called when the metadata of a track has changed. They invoke an MediaDevice DB update */
    virtual void metadataChanged( Meta::TrackPtr track );
    virtual void metadataChanged( Meta::ArtistPtr artist );
    virtual void metadataChanged( Meta::AlbumPtr album );
    virtual void metadataChanged( Meta::GenrePtr genre );
    virtual void metadataChanged( Meta::ComposerPtr composer );
    virtual void metadataChanged( Meta::YearPtr year );

    /**
    * Handler Variables
    */

    Playlists::MediaDeviceUserPlaylistProvider *m_provider; ///< Associated playlist provider

    bool m_copyFailed; ///< Indicates whether a copy failed or not
    bool m_isCopying;
    bool m_isDeleting;

    Meta::TrackList   m_tracksToCopy; ///< List of tracks left to copy
    Meta::TrackList   m_tracksCopying; ///< List of tracks currently copying
    Meta::TrackList   m_tracksToDelete; ///< List of tracks left to delete

    int m_numTracksToCopy; ///< The number of tracks left to copy
    int m_numTracksToRemove; ///< The number of tracks left to remove

    QMap<Meta::TrackPtr, QString> m_tracksFailed; ///< tracks that failed to copy
    QHash<Meta::TrackPtr, Meta::MediaDeviceTrackPtr> m_trackSrcDst; ///< points source to destTracks, for completion of addition to collection

    QMutex m_mutex; ///< A make certain operations atomic when threads are at play

    // Capability-related variables

    Handler::PlaylistCapability *m_pc;
    Handler::PodcastCapability *m_podcastCapability;
    Handler::ReadCapability *m_rc;
    Handler::WriteCapability *m_wc;
};

/**
* The ParseWorkerThread is used to run a full parse of the device's database in
* a separate thread. Once done, it informs the Collection it is done
*/

class ParseWorkerThread : public QObject , public ThreadWeaver::Job
{
    Q_OBJECT
public:
    /**
    * The constructor.
    * @param handler The handler
    */

    explicit ParseWorkerThread( MediaDeviceHandler* handler);

    /**
    * The destructor.
    */

    virtual ~ParseWorkerThread();

    /**
    * Sees the success variable, which says whether or not the copy completed
successfully.
    * @return Whether or not the copy was successful, i.e. m_success
    */

    virtual bool success() const override;

Q_SIGNALS:
    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);
    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed.
     * This signal is emitted when success() returns false after the job is executed. */
    void failed(ThreadWeaver::JobPointer);

private Q_SLOTS:
    /**
    * Is called when the job is done successfully, and simply
    * calls Collection's emitCollectionReady()
    * @param job The job that was done
    */

    void slotDoneSuccess( ThreadWeaver::JobPointer );

protected:
    /**
    * Reimplemented, simply runs the parse method.
    */
    virtual void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

private:
    bool m_success; ///< Whether or not the parse was successful
    MediaDeviceHandler *m_handler; ///< The handler
};

/**
* The CopyWorkerThread is used to run a copy operation on a single track in a separate thread.
* Copying is generally done one thread at a time so as to not hurt performance, and because
* many copying mechanisms like that of libmtp can only copy one file at a time.  Copying
* methods that are not threadsafe should not use CopyWorkerThread, and should set the
* Handler's m_copyingthreadsafe variable to false in the Handler's constructor.
*/

class CopyWorkerThread : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
public:
    /**
    * The constructor.
    * @param track The source track to copy from
    * @param handler The handler
    */

    CopyWorkerThread( const Meta::TrackPtr &track, MediaDeviceHandler* handler );

    /**
    * The destructor.
    */

    virtual ~CopyWorkerThread();

    /**
    * Sets the success variable, which says whether or not the copy completed successfully.
    * @return Whether or not the copy was successful, i.e. m_success
    */

    virtual bool success() const override;

Q_SIGNALS:

    /**
    * Is emitted when the job is done successfully
    * Parameters:
    * The job that was done
    * @param track The source track used for the copy
    */

    void copyTrackDone( ThreadWeaver::JobPointer, const Meta::TrackPtr& track );

    /**
    * Is emitted when the job is done and has failed
    * Parameters:
    * The job that was done
    * @param track The source track used for the copy
    */
    void copyTrackFailed( ThreadWeaver::JobPointer, const Meta::TrackPtr& track );

    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);
    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed.
     * This signal is emitted when success() returns false after the job is executed. */
    void failed(ThreadWeaver::JobPointer);

private Q_SLOTS:
    /**
    * Is called when the job is done successfully, and simply
    * emits copyTrackDone
    * @param job The job that was done
    */

    void slotDoneSuccess( ThreadWeaver::JobPointer );

    /**
    * Is called when the job is done and failed, and simply
    * emits copyTrackFailed
    * @param job The job that was done
    */

    void slotDoneFailed( ThreadWeaver::JobPointer );

protected:
    /**
    * Reimplemented, simply runs the copy track method.
    */
    virtual void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

private:
    bool m_success; ///< Whether or not the copy was successful
    Meta::TrackPtr m_track; ///< The source track to copy from
    MediaDeviceHandler *m_handler; ///< The handler
};

}
#endif
