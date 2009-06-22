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

#include "kjob.h"
#include <KTempDir>
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>

class QString;
class QFile;
class QDateTime;
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

           #if 0

           QMap<Meta::TrackPtr, QString> tracksFailed() const { return m_tracksFailed; } // NOTE: to be used for error-handling later
           #endif
           /** Methods Provided for Collection, i.e. wrappers */

           //virtual void copyTrackListToDevice( const Meta::TrackList tracklist );// NOTE: used by Collection
           //virtual void deleteTrackListFromDevice( const Meta::TrackList &tracks );// NOTE: used by Collection


           /**
            * Parses MediaDevice DB and creates a Meta::MediaDeviceTrack
            * for each track in the DB
            */

        public:
           void parseTracks();// NOTE: used by Collection
           virtual void writeDatabase() = 0;// NOTE: used by Collection

           virtual void copyTrackListToDevice( const Meta::TrackList tracklist );

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

           /// These copy methods are invoked in order:
           /// findPathToCopy
           /// libCopyTrack
           /// libCreateTrack
           /// setBasicMediaDeviceTrackInfo (all the libSets)
           /// addTrackInDB
           /// addMediaDeviceTrackToCollection

           /// findPathToCopy resolves some kind of address to put the track onto on
           /// the device.  For Ipods, this is a url, but for MTPs, this is finding
           /// the right place in the folder structure, so the details of this are
           /// left to the subclass in this method.  NOTE: this function should set
           /// some private variable that can later be used to libSetPlayableUrl

           virtual void findPathToCopy( const Meta::TrackPtr &track ) = 0;

           /// libCopyTrack does the actual file copying.  For Ipods, it uses KIO,
           /// for MTPs this uses a libmtp call
           
           virtual bool libCopyTrack( const Meta::TrackPtr &track ) = 0;

           /// Creates a new track struct particular to the library of the device
           /// e.g. LIBMTP_new_track_t(), and associates it with @param track for
           /// later use

           virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track ) = 0;

           /// Adds the newly created track struct now populated with info into the
           /// database struct of the particular device, e.g. into the itdb for Ipods.
           /// MTP devices automatically add the track into the database upon copying,
           /// so MTP would do nothing.

           virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track ) = 0;

           /// Indicates to the subclass that the database has been updated
           /// For ipods, just sets m_dbchanged = true

           virtual void databaseChanged() {}

           /// Creates a MediaDeviceTrack based on the latest track struct created as a
           /// result of a copy to the device, and adds it into the collection to reflect
           /// that it has been copied.

           void addMediaDeviceTrackToCollection( Meta::MediaDeviceTrackPtr &track );

           /// setCopyTrackForParse makes it so that a call to getBasicMediaDeviceTrackInfo
           /// will use the track struct recently created and filled with info, to fill up
           /// the Meta::MediaDeviceTrackPtr.

           virtual void setCopyTrackForParse() = 0;

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

           signals:
           void copyTracksDone( bool success );
           void libCopyTrackDone( const Meta::TrackPtr &track );
           void libCopyTrackFailed( const Meta::TrackPtr &track );
           void canCopyMoreTracks(); // subclass tells this class that it can copy more tracks
           #if 0
           void updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track, Itdb_Track *existingMediaDeviceTrack );// NOTE: used by Collection

        public slots:
           bool initializeMediaDevice();

        private:
           /* Handler's Main Methods */

           // NOTE: do not use writeDB,
           // use the threaded writeDatabase
           // TODO: declare a friend, the thread that runs this
           virtual bool writeDB( bool threaded=false );

           virtual QPixmap getCover( Meta::MediaDeviceTrackPtr track ) const;

           virtual bool setCoverArt( MediaDeviceTrackPtr track, const QString &filename ) const;
           virtual bool setCoverArt( MediaDeviceTrackPtr track, const QPixmap &image ) const;

           virtual void setRating( const int newrating );

           /**
            * @param ipodtrack - track being read from
            * @param track - track being written to
            * Extracts track information from ipodtrack
            * and puts it in track
            */
           void getBasicMediaDeviceTrackInfo( const Itdb_Track *ipodtrack, Meta::MediaDeviceTrackPtr track ) const;

           /* Handler's Collection Methods */

           void addMediaDeviceTrackToCollection( Itdb_Track *ipodtrack );

           /* libgpod DB Methods */

           void addTrackInDB( Itdb_Track *ipodtrack );
           void insertTrackIntoDB( const KUrl &url, const Meta::TrackPtr &track );
           bool removeDBTrack( Itdb_Track *track );

           /* Cover Art functions */
           QString ipodArtFilename( const Itdb_Track *ipodtrack ) const;
           void getCoverArt( const Itdb_Track *ipodtrack );

           /* File I/O Methods */

           void copyTracksToDevice();

           void copyNextTrackToDevice();
           void deleteNextTrackFromDevice();

           void privateCopyTrackToDevice( const Meta::TrackPtr &track );
           void privateDeleteTrackFromDevice( const Meta::TrackPtr &track );

           void deleteFile( const KUrl &url );
           bool kioCopyTrack( const KUrl &src, const KUrl &dst );

           /* Convenience methods to avoid repetitive code */

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

        private:


           /// This function is called just before copying tracks begin and allows
           /// a subclass to prepare to copy, e.g. for Ipods it would initialize
           /// the job counter to 0.
           virtual void prepareToCopy() {}



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

    void slotCopyNextTrackFailed( ThreadWeaver::Job* job );
    void slotCopyNextTrackDone( ThreadWeaver::Job* job, const Meta::TrackPtr& track );

    void slotCopyTrackJobsDone( ThreadWeaver::Job* job );
    void slotFinalizeTrackCopy( const Meta::TrackPtr & track );
    void slotCopyTrackFailed( const Meta::TrackPtr & track );

    void slotDatabaseWritten( bool success );

           /* Collection Variables */
        private:
           // Associated collection
           MediaDeviceCollection   *m_memColl;
           // Map of titles, used to check for duplicate tracks
#if 0
           TitleMap          m_titlemap;

           /* libgpod variables */
           Itdb_iTunesDB    *m_itdb;
           Itdb_Playlist    *m_masterPlaylist;

           /* Lockers */
           QMutex            m_dbLocker; // DB only written by 1 thread at a time
           QMutex            m_joblocker; // lets only 1 job finish at a time
           int               m_jobcounter; // keeps track of copy jobs present

           /* Copy/Delete Variables */
           Meta::TrackList   m_tracksToCopy;
           Meta::TrackList   m_tracksToDelete;

           /* Operation Progress Bar */
           ProgressBar      *m_statusbar;

           /* MediaDevice Connection */
           bool              m_autoConnect;
           QString           m_mountPoint;
           QString           m_name;

           /* MediaDevice Model */
           bool              m_isShuffle;
           bool              m_isMobile;
           bool              m_isIPhone;

           /* Properties of MediaDevice */
           bool              m_supportsArtwork;
           bool              m_supportsVideo;
           bool              m_rockboxFirmware;
           bool              m_needsFirewireGuid;

           /* Success/Failure */
           bool m_dbChanged;
           bool m_copyFailed;
           bool m_isCanceled;
           bool m_wait;
           // whether Itdb_Track is created correctly
           bool m_trackCreated;
           // whether read MediaDevice DB or not
           bool m_success;

           /* Miscellaneous Variables */

           // tracks that failed to copy

           QMap<Meta::TrackPtr, QString> m_tracksFailed;

           // tempdir for covers
           KTempDir *m_tempdir;
           QSet<QString> m_coverArt;

        private slots:
          void fileTransferred( KJob *job );
          void fileDeleted( KJob *job );

          void slotDBWriteFailed( ThreadWeaver::Job* job );
          void slotDBWriteSucceeded( ThreadWeaver::Job* job );

#endif

        bool m_copyFailed;

        int m_numTracksToCopy;

           // tracks that failed to copy

           QMap<Meta::TrackPtr, QString> m_tracksFailed;

           Meta::TrackList   m_tracksToCopy;
           Meta::TrackList   m_tracksCopying;
           Meta::TrackPtr m_lastTrackCopied;

           TitleMap          m_titlemap;

           ProgressBar      *m_statusbar;

           QMutex m_mutex;

        protected:
            bool m_success;
            bool m_copyingthreadsafe; // whether or not the handler's method of copying is threadsafe
    };

#if 0
    class DBWorkerThread : public ThreadWeaver::Job
    {
        Q_OBJECT
        public:
            DBWorkerThread( MediaDeviceHandler* handler );
            virtual ~DBWorkerThread();

            virtual bool success() const;

        protected:
            virtual void run();

        private:
            bool m_success;
            MediaDeviceHandler *m_handler;
    };
#endif



class CopyWorkerThread : public ThreadWeaver::Job
{
    Q_OBJECT
public:
    CopyWorkerThread( const Meta::TrackPtr &track, MediaDeviceHandler* handler );
    virtual ~CopyWorkerThread();

    virtual bool success() const;

    signals:
        void copyTrackDone( ThreadWeaver::Job*, const Meta::TrackPtr& track);

    private slots:
        void slotDone( ThreadWeaver::Job* );

protected:
    virtual void run();

private:
    bool m_success;
    Meta::TrackPtr m_track;
    MediaDeviceHandler *m_handler;
};

}
#endif
