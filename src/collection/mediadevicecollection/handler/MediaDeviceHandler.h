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
//#include <threadweaver/Job.h>

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
            void databaseWritten( bool success );

           #if 0

           QMap<Meta::TrackPtr, QString> tracksFailed() const { return m_tracksFailed; } // NOTE: to be used for error-handling later

           /** Methods Provided for Collection, i.e. wrappers */

           virtual void copyTrackListToDevice( const Meta::TrackList tracklist );// NOTE: used by Collection
           virtual void deleteTrackListFromDevice( const Meta::TrackList &tracks );// NOTE: used by Collection

           #endif
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


           /// Methods that wrap get/set of information using given library (e.g. libgpod)
           /// Subclasses of MediaDeviceHandler must keep a pointer to the current
           /// track in question, and these methods are executed on that track

           virtual QString libGetTitle() const = 0;
           virtual QString libGetAlbum() const = 0;
           virtual QString libGetArtist() const = 0;
           virtual QString libGetComposer() const = 0;
           virtual QString libGetGenre() const = 0;
           virtual int libGetYear() const = 0;
           virtual int     libGetLength() const = 0;
           virtual int     libGetTrackNumber() const = 0;
           virtual QString libGetComment() const = 0;
           virtual int     libGetDiscNumber() const = 0;
           virtual int     libGetBitrate() const = 0;
           virtual int     libGetSamplerate() const = 0;
           virtual float   libGetBpm() const = 0;
           virtual int     libGetFileSize() const = 0;
           virtual int     libGetPlayCount() const = 0;
           virtual uint    libGetLastPlayed() const = 0;
           virtual int     libGetRating() const  = 0;
           virtual QString libGetType() const = 0;
           virtual QString libGetPlayableUrl() const = 0;

           /// Uses wrapped methods to fill a track with information from device
           void getBasicMediaDeviceTrackInfo( Meta::MediaDeviceTrackPtr track ) const;

           signals:
           void copyTracksDone( bool success );
           #if 0
           void updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track, Itdb_Track *existingMediaDeviceTrack );// NOTE: used by Collection




        signals:
           void deleteTracksDone();// NOTE: used by Collection
           void incrementProgress();
           void endProgressOperation( const QObject *owner );

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

           /* libgpod Information Extraction Methods */

           void detectModel();
           KUrl determineURLOnDevice( const Meta::TrackPtr &track );
           QString itunesDir( const QString &path = QString() ) const;
           QString ipodPath( const QString &realPath );
           bool pathExists( const QString &ipodPath, QString *realPath=0 );
           QString realPath( const char *ipodPath );

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

           #if 0

           /* Observer Methods */

           /** These methods are called when the metadata of a track has changed. They invoke an MediaDevice DB update */
           virtual void metadataChanged( Meta::TrackPtr track );
           virtual void metadataChanged( Meta::ArtistPtr artist );
           virtual void metadataChanged( Meta::AlbumPtr album );
           virtual void metadataChanged( Meta::GenrePtr genre );
           virtual void metadataChanged( Meta::ComposerPtr composer );
           virtual void metadataChanged( Meta::YearPtr year );

#endif
           /**
            * Handler Variables
            */

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

        protected:
            bool m_success;
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

}

#endif
