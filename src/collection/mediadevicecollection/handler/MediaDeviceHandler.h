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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MEDIADEVICEHANDLER_H
#define MEDIADEVICEHANDLER_H

#include "MediaDeviceMeta.h"
#include "MemoryCollection.h"
#include "../../../statusbar/StatusBar.h"

#include "mediadevicecollection_export.h"

#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>

class QString;
class QMutex;

class MediaDeviceCollection;

class MediaDeviceUserPlaylistProvider;

namespace Meta
{

typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

/**
The MediaDeviceHandler is the backend where all low-level library calls are made.
It exists to leave a generic API in the other classes, while allowing for low-level
calls to be isolated here.
*/

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceHandler : public QObject, public Meta::Observer
{
    Q_OBJECT

    public:

    /**
    * Constructor
    */

    MediaDeviceHandler( QObject *parent );

    /**
    * Destructor
    */

    virtual ~MediaDeviceHandler();

    /**
    * Begins an attempt to connect to the device, and emits
    * attemptConnectionDone when it finishes.
    */

    virtual void init() = 0; // begin connection atempts

    /**
    * Checks if a device can be written to.
    * @return
    *   TRUE if the device can be written to
    *   FALSE if the device can not be written to
    */

    virtual bool isWritable() const;

    /**
    * Checks if the handler successfully connected
    * to the device.
    * @return
    *   TRUE if the device was successfully connected to
    *   FALSE if the device was not successfully connected to
    */

    bool succeeded() const
    {
        return m_success;
    }

    /// Methods provided for CollectionLocation

    /** Given a list of tracks, get URLs for device tracks
    *  of this type of device.  If the device needs to
    *  do some work to get URLs (e.g. copy tracks to a
    *  temporary location) the overridden method in
    *  the handler takes care of it.
    *  @param tracks The list of tracks for which to fetch urls
    */

    virtual void getCopyableUrls( const Meta::TrackList &tracks );

    /**
    * Parses the media device's database and creates a Meta::MediaDeviceTrack
    * for each track in the database.
    */

    void parseTracks();

    /**
    * Writes to the device's database if it has one, otherwise
    * simply calls slotDatabaseWritten to continue the workflow.
    */

    virtual void writeDatabase()
    {
        slotDatabaseWritten( true );
    }

    /**
    * Fetches the human-readable name of the device.
    * This is often called from the Collection since
    * a library call is needed to get this name.
    * @return A QString with the name
    */

    virtual QString prettyName() const
    {
        return QString();
    }

    /**
    * Copies a list of tracks to the device.
    * @param tracklist The list of tracks to copy.
    */

    virtual void copyTrackListToDevice( const Meta::TrackList tracklist );

    /**
    * Removes a list of tracks from the device.
    * @param tracklist The list of tracks to remove.
    */

    virtual void removeTrackListFromDevice( const Meta::TrackList &tracks );

    /* Parsing of Tracks on Device */

    /** Initializes iteration over some list of track structs
    *  e.g. with libgpod, this initializes a GList to the beginning of
    *  the list of tracks
    */

    virtual void prepareToParseTracks() = 0;

    /** Runs a test to see if we have reached the end of
    *  the list of tracks to be parsed on the device, e.g. in libgpod
    *  this tests if cur != NULL, i.e. if(cur)
    */

    virtual bool isEndOfParseTracksList() = 0;

    /** Moves the iterator to the next track on the list of
    *  track structs, e.g. with libgpod, cur = cur->next where cur
    *  is a GList*
    */

    virtual void prepareToParseNextTrack() = 0;

    /** This method attempts to access the special struct of the
    *  next track, so that information can then be parsed from it.
    * For libgpod, this is m_currtrack = (Itdb_Track*) cur->data
    */

    virtual void nextTrackToParse() = 0;

    /** This method must create a two-way association of the current Meta::Track
    *  to the special struct provided by the library to read/write information.
    *  For example, for libgpod one would associate Itdb_Track*.  It makes
    *  the most sense to use a QHash since it is fastest lookup and order
    *  does not matter.
    *  @param track The track to two-way associate with a library track struct
    */

    virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track ) = 0;

    /// Parsing of Tracks in Playlists on Device
    /// NOTE: not required by devices with no playlists

    /** This method initializes iteration over some list of playlist structs
    *  e.g. with libgpod, this initializes a GList to the beginning of
    *  the list of playlists
    */

    virtual void prepareToParsePlaylists() {}

    /** This method runs a test to see if we have reached the end of
    *  the list of playlists to be parsed on the device, e.g. in libgpod
    *  this tests if cur != NULL, i.e. if(cur)
    */

    virtual bool isEndOfParsePlaylistsList()
    {
        return true;
    }

    /** This method moves the iterator to the next playlist on the list of
    *  playlist structs, e.g. with libgpod, cur = cur->next where cur
    *  is a GList*
    */

    virtual void prepareToParseNextPlaylist() {}

    /** This method attempts to access the special struct of the
    *  next playlist, so that information can then be parsed from it.
    *  For libgpod, this is m_currplaylist = ( Itdb_Playlist * ) cur->data
    */

    virtual void nextPlaylistToParse() {}

    /** This method checks if the playlist should be parsed, or skipped.
    *  Certain playlists, like the master playlist on the iPod, do not
    *  need to be or should not be parsed.
    *  @return true if should not parse, false otherwise.
    */

    virtual bool shouldNotParseNextPlaylist()
    {
        return false;
    }

    /** This method initializes iteration over some list of track structs
    *  that correspond to a playlist struct
    *  e.g. with libgpod, this initializes a GList to the beginning of
    *  the list of tracks
    */

    virtual void prepareToParsePlaylistTracks() {}

    /** This method runs a test to see if we have reached the end of
    *  the list of tracks in the playlist to be parsed on the device, e.g. in libgpod
    *  this tests if cur != NULL, i.e. if(cur)
    */

    virtual bool isEndOfParsePlaylist()
    {
        return true;
    }

    /** This method moves the iterator to the next track on the playlist of
    *  track structs, e.g. with libgpod, cur = cur->next where cur
    *  is a GList*
    */

    virtual void prepareToParseNextPlaylistTrack() {}

    /** This method attempts to access the special struct of the
    *  next track on the playlist, so that information can then be parsed from it.
    *  For libgpod, this is m_currtrack = (Itdb_Track*) cur->data
    */

    virtual void nextPlaylistTrackToParse() {}

    /** Returns a list of formats supported by the device, all in lowercase
    *  For example mp3, mpeg, aac.  This is used to avoid copying unsupported
    *  types to a particular device.
    */

    virtual QStringList supportedFormats() = 0;

    /**  The copy methods are invoked in order:
    *  libCopyTrack
    *  libCreateTrack
    *  setBasicMediaDeviceTrackInfo (all the libSets)
    *  addTrackInDB
    *  addMediaDeviceTrackToCollection

    *  The remove methods are invoked in order:
    *  libDeleteTrackFile
    *  removeTrackFromDB
    *  libDeleteTrack
    *  removeMediaDeviceTrackFromCollection
    */

    /**
    * Finds the place to copy the track to on the device, which
    * could be a url in the case of Ipods, or a folder in the
    * case of MTP devices.
    * @param track The source track of the copy
    */

    virtual void findPathToCopy( const Meta::TrackPtr &track )
    {
        Q_UNUSED( track )
    }

    /** libCopyTrack does the actual file copying.  For Ipods, it uses KIO,
    *  for MTPs this uses a libmtp call
    *  Copy the file associate with srcTrack to destTrack
    *  @param srcTrack The track being copied from
    *  @param destTrack The track being copied to
    *  @return Whether or not the track copy was successful
    */

    virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
    {
        Q_UNUSED( srcTrack ) Q_UNUSED( destTrack ) return false;
    }

    /** libDeleteTrack does the actual file deleting.  For Ipods, it uses KIO,
    *  for MTPs this uses a libmtp call.
    *  @param track The track whose file is to be deleted
    */

    virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
    {
        emit libRemoveTrackDone( Meta::TrackPtr::staticCast( track ) ); return false;
    }

    /** Creates a new track struct particular to the library of the device
    *  e.g. LIBMTP_new_track_t(), and associates it with the track for
    *  later use, in the same way that setAssociateTrack does it.
    *  @param track The track for which to create a track struct and associate it to
    */

    virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /** Deletes the track struct associated with this track, freeing
    *  any memory it occupied, and dissociating it from the track
    *  @param track The track whose associated track struct is to be deleted.
    */

    virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /** Returns a MediaDeviceTrackPtr that is associated with the currently parsed track struct.
    *  This is mainly used in playlist parsing, and can be ignored otherwise.
    *  @return A MediaDeviceTrackPtr to currently parsed track struct
    */

    virtual Meta::MediaDeviceTrackPtr libGetTrackPtrForTrackStruct()
    {
        return Meta::MediaDeviceTrackPtr();
    }

    /** Returns a string containing the playlist name of the currently parsed playlist struct, if available.
    *  Only override if your library uses names.
    *  @return A string with the name of the currently parsed playlist
    */

    virtual QString libGetPlaylistName()
    {
        return QString();
    }

    /** Adds the newly created track struct now populated with info into the
    *  database struct of the particular device, e.g. into the itdb for Ipods.
    *  MTP devices automatically add the track into the database upon copying,
    *  so MTP would do nothing.
    *  @param track The track whose associated track struct is to be added \
    into the database.
    */

    virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track )
    {
        Q_UNUSED( track )
    }

    /** Remove all traces of the track struct associated with @param track from
    *  the database struct, but do not delete the struct
    *  @param track The track whose associated track struct is to be removed \
    from the database.
    */

    virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track )
    {
        Q_UNUSED( track )
    }

    /** Indicates to the subclass that the database has been updated
    *  For ipods, just sets m_dbchanged = true
    */

    virtual void databaseChanged() {}

    /** Creates a MediaDeviceTrack based on the latest track struct created as a
    *  result of a copy to the device, and adds it into the collection to reflect
    *  that it has been copied.
    *  @param track The track to add to the collection
    */

    void addMediaDeviceTrackToCollection( Meta::MediaDeviceTrackPtr &track );

    /**  Removes the @param track from all the collection's maps to reflect that
    *  it has been removed from the collection
    *  @param track The track to remove from the collection
    */

    void removeMediaDeviceTrackFromCollection( Meta::MediaDeviceTrackPtr &track );

    /** Uses wrapped libGet methods to fill a track with information from device
    *  @param track The track from whose associated struct to get the information
    *  @param destTrack The track that we want to fill with information
    */
    void getBasicMediaDeviceTrackInfo( const Meta::MediaDeviceTrackPtr& track, Meta::MediaDeviceTrackPtr destTrack );

    /** Uses wrapped libSet methods to fill a track struct of the particular library
    *  with information from a Meta::Track
    *  @param srcTrack The track that has the source information
    *  @param destTrack The track whose associated struct we want to fill with information
    */

    void setBasicMediaDeviceTrackInfo( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr destTrack );

    /** Methods that wrap get/set of information using given library (e.g. libgpod)
    *  Subclasses of MediaDeviceHandler must keep a pointer to the track struct
    *  associated to the track parameter to get the information from the struct in libGet*,
    *  and to set the struct's information to the passed metadata in libSet*
    */

    virtual QString libGetTitle( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetLength( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual QString libGetComment( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetBitrate( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetSamplerate( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual float   libGetBpm( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetFileSize( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetPlayCount( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual uint    libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual int     libGetRating( const Meta::MediaDeviceTrackPtr &track )  = 0;
    virtual QString libGetType( const Meta::MediaDeviceTrackPtr &track ) = 0;
    virtual QString libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track ) = 0;

    /**  Each libSet function sets the private track struct associated with @param track
    *    to the second value passed into the function.
    */

    virtual void    libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title ) = 0;
    virtual void    libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album ) = 0;
    virtual void    libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist ) = 0;
    virtual void    libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer ) = 0;
    virtual void    libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre ) = 0;
    virtual void    libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year ) = 0;
    virtual void    libSetLength( Meta::MediaDeviceTrackPtr &track, int length ) = 0;
    virtual void    libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum ) = 0;
    virtual void    libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment ) = 0;
    virtual void    libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum ) = 0;
    virtual void    libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate ) = 0;
    virtual void    libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate ) = 0;
    virtual void    libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm ) = 0;
    virtual void    libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize ) = 0;
    virtual void    libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount ) = 0;
    virtual void    libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed ) = 0;
    virtual void    libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )  = 0;
    virtual void    libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type ) = 0;
    virtual void    libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack ) = 0;

    // TODO: NYI
    //virtual void    libSetCoverArt( Meta::MediaDeviceTrackPtr &track, const QPixmap& image ) {}

    /** This function is called just before a track in the playlist is to be played, and gives
    *  a chance for e.g. MTP to copy the track to a temporary location, set a playable url,
    *  to emulate the track actually being played off the device
    *  @param track The track that needs to prepare to be played
    */

    virtual void prepareToPlay( Meta::MediaDeviceTrackPtr &track )
    {
        Q_UNUSED( track )
    }

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
    void setupAlbumMap( Meta::MediaDeviceTrackPtr track, AlbumMap &albumMap );
    void setupGenreMap( Meta::MediaDeviceTrackPtr track, GenreMap &genreMap );
    void setupComposerMap( Meta::MediaDeviceTrackPtr track, ComposerMap &composerMap );
    void setupYearMap( Meta::MediaDeviceTrackPtr track, YearMap &yearMap );

signals:
    void gotCopyableUrls( const QMap<Meta::TrackPtr, KUrl> &urls );
    void databaseWritten( bool succeeded );

    void deleteTracksDone();
    void incrementProgress();
    void endProgressOperation( const QObject *owner );

    void copyTracksDone( bool success );
    void removeTracksDone();
    void libCopyTrackDone( const Meta::TrackPtr &track );
    void libCopyTrackFailed( const Meta::TrackPtr &track );
    void libRemoveTrackDone( const Meta::TrackPtr &track );
    void canCopyMoreTracks(); // subclass tells this class that it can copy more tracks
    void canDeleteMoreTracks(); // subclass tells this class that it can delete more tracks

    void attemptConnectionDone( bool success );

    /* File I/O Methods */

public slots:

    void copyNextTrackToDevice();
    bool privateCopyTrackToDevice( const Meta::TrackPtr& track );

    void removeNextTrackFromDevice();
    void privateRemoveTrackFromDevice( const Meta::TrackPtr &track );


protected:
    bool m_success;
    bool m_copyingthreadsafe; // whether or not the handler's method of copying is threadsafe


private slots:

    void slotCopyNextTrackFailed( ThreadWeaver::Job* job, const Meta::TrackPtr& track );
    void slotCopyNextTrackDone( ThreadWeaver::Job* job, const Meta::TrackPtr& track );

    void slotCopyTrackJobsDone( ThreadWeaver::Job* job );
    void slotFinalizeTrackCopy( const Meta::TrackPtr & track );
    void slotCopyTrackFailed( const Meta::TrackPtr & track );
    void slotFinalizeTrackRemove( const Meta::TrackPtr & track );

    void slotDatabaseWritten( bool success );

    void enqueueNextCopyThread();

    void slotDeletingHandler();

private:


    /** This function is called just before copying tracks begin and allows
    *  a subclass to prepare to copy, e.g. for Ipods it would initialize
    *  the job counter to 0.
    */
    virtual void prepareToCopy() {}

    /** This function is called just before deleting tracks begin and allows
    *  a subclass to prepare to delete, e.g. for Ipods it would initialize
    *  the m_tracksdeleting to keep track of urls it is deleting.
    */

    virtual void prepareToDelete() {}

    /** Tells subclass that it can update the track, usually because
    *  the track's tags have changed.
    *  @param track The track whose tags should be updated
    */

    virtual void updateTrack( Meta::MediaDeviceTrackPtr &track )
    {
        Q_UNUSED( track )
    }

    /// Observer Methods

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

    /* Collection Variables */

    MediaDeviceCollection   *m_memColl; /// Associated collection
    MediaDeviceUserPlaylistProvider *m_provider; /// Associated playlist provider

    bool m_copyFailed; /// Indicates whether a copy failed or not

    int m_numTracksToCopy; /// The number of tracks left to copy
    int m_numTracksToRemove; /// The number of tracks left to remove



    QMap<Meta::TrackPtr, QString> m_tracksFailed; /// tracks that failed to copy
    QHash<Meta::TrackPtr, Meta::MediaDeviceTrackPtr> m_trackSrcDst; /// points source to destTracks, for completion of addition to collection

    Meta::TrackList   m_tracksToCopy; /// List of tracks left to copy
    Meta::TrackList   m_tracksCopying; /// List of tracks currrently copying

    Meta::TrackList   m_tracksToDelete; /// List of tracks left to delete

    TitleMap          m_titlemap; /// Map of track titles to tracks, used to detect duplicates

    ProgressBar      *m_statusbar; /// A progressbar to show progress of an operation

    QMutex m_mutex; /// A make certain operations atomic when threads are at play

};

/**
* The CopyWorkerThread is used to run a copy operation on a single track in a separate thread.
* Copying is generally done one thread at a time so as to not hurt performance, and because
* many copying mechanisms like that of libmtp can only copy one file at a time.  Copying
* methods that are not threadsafe should not use CopyWorkerThread, and should set the
* Handler's m_copyingthreadsafe variable to false in the Handler's constructor.
*/

class CopyWorkerThread : public ThreadWeaver::Job
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

    virtual bool success() const;

signals:

    /**
    * Is emitted when the job is done successfully
    * @param job The job that was done
    * @param track The source track used for the copy
    */

    void copyTrackDone( ThreadWeaver::Job*, const Meta::TrackPtr& track );

    /**
    * Is emitted when the job is done and has failed
    * @param job The job that was done
    * @param track The source track used for the copy
    */
    void copyTrackFailed( ThreadWeaver::Job*, const Meta::TrackPtr& track );

private slots:
    /**
    * Is called when the job is done successfully, and simply
    * emits copyTrackDone
    * @param job The job that was done
    */

    void slotDoneSuccess( ThreadWeaver::Job* );

    /**
    * Is called when the job is done and failed, and simply
    * emits copyTrackFailed
    * @param job The job that was done
    */

    void slotDoneFailed( ThreadWeaver::Job* );

protected:
    /**
    * Reimplemented, simply runs the copy track method.
    */
    virtual void run();

private:
    bool m_success; /// Whether or not the copy was successful
    Meta::TrackPtr m_track; /// The source track to copy from
    MediaDeviceHandler *m_handler; /// The handler
};

}
#endif
