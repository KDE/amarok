/***************************************************************************
 * copyright            : (C) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

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

namespace Meta {

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
	MediaDeviceHandler( QObject *parent );
	virtual ~MediaDeviceHandler() {}

           /** Status Checking Methods */

           /// Device can be written to?

           virtual bool isWritable() const;

           /**
           * Successfully read MediaDevice database?
           */
           // NOTE: used by Collection
           // TODO: to be replaced when connection of device gets thread support
           bool succeeded() const { return m_success; }

           /// Methods provided for CollectionLocation

        /// Given a list of tracks, get URLs for device tracks
        /// of this type of device.  If the device needs to
        /// do some work to get URLs (e.g. copy tracks to a
        /// temporary location) the overridden method in
        /// the handler takes care of it
        /// Default implementation: 
           virtual void getCopyableUrls( const Meta::TrackList &tracks );


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

           /**
            * Parses MediaDevice DB and creates a Meta::MediaDeviceTrack
            * for each track in the DB
            */

        public:
           void parseTracks();// NOTE: used by Collection
           virtual void writeDatabase() { slotDatabaseWritten( true );}// NOTE: used by Collection

           virtual QString prettyName() const { return QString(); } // NOTE: used by Collection

           virtual void copyTrackListToDevice( const Meta::TrackList tracklist );
           virtual void removeTrackListFromDevice( const Meta::TrackList &tracks );

           /// This method initializes iteration over some list of track structs
           /// e.g. with libgpod, this initializes a GList to the beginning of
           /// the list of tracks

           virtual void prepareToParse() = 0;

           /// This method runs a test to see if we have reached the end of
           /// the list of tracks to be parsed on the device, e.g. in libgpod
           /// this tests if cur != NULL, i.e. if(cur)

           virtual bool isEndOfParseList() = 0;

           /// This method moves the iterator to the next track on the list of
           /// track structs, e.g. with libgpod, cur = cur->next where cur
           /// is a GList*

           virtual void prepareToParseNextTrack() = 0;

           /// This method attempts to access the special struct of the
           /// next track, so that information can then be parsed from it.
           /// For libgpod, this is m_currtrack = (Itdb_Track*) cur->data

           virtual void nextTrackToParse() = 0;

           /// This method must create an association of the current Meta::Track
           /// to the special struct provided by the library to read/write information.
           /// For example, for libgpod one would associate Itdb_Track*.  It makes
           /// the most sense to use a QHash since it is fastest lookup and order
           /// does not matter.

           virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track ) = 0;

           /// Returns a list of formats supported by the device, all in lowercase
           /// For example mp3, mpeg, aac.  This is used to avoid copying unsupported
           /// types to a particular device.

           virtual QStringList supportedFormats() = 0;

           /// The copy methods are invoked in order:
           /// libCopyTrack
           /// libCreateTrack
           /// setBasicMediaDeviceTrackInfo (all the libSets)
           /// addTrackInDB
           /// addMediaDeviceTrackToCollection

           /// The remove methods are invoked in order:
           /// libDeleteTrackFile
           /// removeTrackFromDB
           /// libDeleteTrack
           /// removeMediaDeviceTrackFromCollection

	   virtual void findPathToCopy( const Meta::TrackPtr &track ) { Q_UNUSED( track ) }

           /// libCopyTrack does the actual file copying.  For Ipods, it uses KIO,
           /// for MTPs this uses a libmtp call
           /// Copy the file associate with srcTrack to destTrack
           
           virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack ) { Q_UNUSED( srcTrack ) Q_UNUSED( destTrack ) return false; }

           /// libDeleteTrack does the actual file deleting.  For Ipods, it uses KIO,
           /// for MTPs this uses a libmtp call.

           virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) return false; }

           /// Creates a new track struct particular to the library of the device
           /// e.g. LIBMTP_new_track_t(), and associates it with @param track for
           /// later use

           virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track ) = 0;

           /// Deletes the track struct associated with this @param track, freeing
           /// any memory it occupied, and dissociating it from the @param track

           virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track ) = 0;

           /// Adds the newly created track struct now populated with info into the
           /// database struct of the particular device, e.g. into the itdb for Ipods.
           /// MTP devices automatically add the track into the database upon copying,
           /// so MTP would do nothing.

           virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) }

           /// Remove all traces of the track struct associated with @param track from
           /// the database struct, but do not delete the struct

           virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) }

           /// Indicates to the subclass that the database has been updated
           /// For ipods, just sets m_dbchanged = true

           virtual void databaseChanged() {}

           /// Creates a MediaDeviceTrack based on the latest track struct created as a
           /// result of a copy to the device, and adds it into the collection to reflect
           /// that it has been copied.

           void addMediaDeviceTrackToCollection( Meta::MediaDeviceTrackPtr &track );

           /// Removes the @param track from all the collection's maps to reflect that
           /// it has been removed from the collection

           void removeMediaDeviceTrackFromCollection( Meta::MediaDeviceTrackPtr &track );

           /// Uses wrapped libGet methods to fill a track with information from device
           void getBasicMediaDeviceTrackInfo( const Meta::MediaDeviceTrackPtr& track, Meta::MediaDeviceTrackPtr destTrack );

           /// Uses wrapped libSet methods to fill a track struct of the particular library
           /// with information from a Meta::Track

           void setBasicMediaDeviceTrackInfo( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr destTrack );

           /// Methods that wrap get/set of information using given library (e.g. libgpod)
           /// Subclasses of MediaDeviceHandler must keep a pointer to the current
           /// track in question, and these methods are executed on that track

           /// get the required information from the track struct associated with the @param track

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



           /// Each libSet function sets the private track struct associated with @param track
           /// to the second value passed into the function.

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
           virtual void    libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed) = 0;
           virtual void    libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )  = 0;
           virtual void    libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type ) = 0;
           virtual void    libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack ) = 0;

           //virtual void    libSetCoverArt( Meta::MediaDeviceTrackPtr &track, const QPixmap& image ) {}

           /// This function is called just before a track in the playlist is to be played, and gives
           /// a chance for e.g. MTP to copy the track to a temporary location, set a playable url,
           /// to emulate the track actually being played off the device

           virtual void prepareToPlay( Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) }

      
           #if 0

        public slots:
           bool initializeMediaDevice();

        private:
           /* Handler's Main Methods */

           virtual QPixmap getCover( Meta::MediaDeviceTrackPtr track ) const;

           virtual bool setCoverArt( MediaDeviceTrackPtr track, const QString &filename ) const;
           virtual bool setCoverArt( MediaDeviceTrackPtr track, const QPixmap &image ) const;

           /* Cover Art functions */
           QString ipodArtFilename( const Itdb_Track *ipodtrack ) const;
           void getCoverArt( const Itdb_Track *ipodtrack );

           #endif
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

        public:
           void setupArtistMap( Meta::MediaDeviceTrackPtr track, ArtistMap &artistMap );
           void setupAlbumMap( Meta::MediaDeviceTrackPtr track, AlbumMap &albumMap );
           void setupGenreMap( Meta::MediaDeviceTrackPtr track, GenreMap &genreMap );
           void setupComposerMap( Meta::MediaDeviceTrackPtr track, ComposerMap &composerMap );
           void setupYearMap( Meta::MediaDeviceTrackPtr track, YearMap &yearMap );

           /* File I/O Methods */

        public slots:

           void copyNextTrackToDevice();
           bool privateCopyTrackToDevice( const Meta::TrackPtr& track );

           void removeNextTrackFromDevice();
           void privateRemoveTrackFromDevice( const Meta::TrackPtr &track );

        private:


           /// This function is called just before copying tracks begin and allows
           /// a subclass to prepare to copy, e.g. for Ipods it would initialize
           /// the job counter to 0.
           virtual void prepareToCopy() {}

           /// This function is called just before deleting tracks begin and allows
           /// a subclass to prepare to delete, e.g. for Ipods it would initialize
           /// the m_tracksdeleting to keep track of urls it is deleting.

           virtual void prepareToDelete() {}

           /* Observer Methods */

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

           private slots:

    void slotCopyNextTrackFailed( ThreadWeaver::Job* job, const Meta::TrackPtr& track );
    void slotCopyNextTrackDone( ThreadWeaver::Job* job, const Meta::TrackPtr& track );

    void slotCopyTrackJobsDone( ThreadWeaver::Job* job );
    void slotFinalizeTrackCopy( const Meta::TrackPtr & track );
    void slotCopyTrackFailed( const Meta::TrackPtr & track );
    void slotFinalizeTrackRemove( const Meta::TrackPtr & track );

    void slotDatabaseWritten( bool success );

    void enqueueNextCopyThread();

           /* Collection Variables */
        private:
           // Associated collection
           MediaDeviceCollection   *m_memColl;

        bool m_copyFailed;

        int m_numTracksToCopy;
        int m_numTracksToRemove;

           // tracks that failed to copy

           QMap<Meta::TrackPtr, QString> m_tracksFailed;
           QHash<Meta::TrackPtr, Meta::MediaDeviceTrackPtr> m_trackSrcDst; // points source to destTracks, for completion of addition to collection

           Meta::TrackList   m_tracksToCopy;
           Meta::TrackList   m_tracksCopying;
           Meta::TrackPtr m_lastTrackCopied;

           Meta::TrackList   m_tracksToDelete;

           TitleMap          m_titlemap;

           ProgressBar      *m_statusbar;

           QMutex m_mutex;

        protected:
            bool m_success;
            bool m_copyingthreadsafe; // whether or not the handler's method of copying is threadsafe
    };

class CopyWorkerThread : public ThreadWeaver::Job
{
    Q_OBJECT
public:
    CopyWorkerThread( const Meta::TrackPtr &track, MediaDeviceHandler* handler );
    virtual ~CopyWorkerThread();

    virtual bool success() const;

    signals:
        void copyTrackDone( ThreadWeaver::Job*, const Meta::TrackPtr& track);
        void copyTrackFailed( ThreadWeaver::Job*, const Meta::TrackPtr& track);

    private slots:
        void slotDoneSuccess( ThreadWeaver::Job* );
        void slotDoneFailed( ThreadWeaver::Job* );

protected:
    virtual void run();

private:
    bool m_success;
    Meta::TrackPtr m_track;
    MediaDeviceHandler *m_handler;
};

}
#endif
