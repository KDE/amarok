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

#include "Meta.h"
//#include "MemoryCollection.h"
//#include "MediaDeviceMeta.h"
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

           /**
           * Successfully read MediaDevice database?
           */
           // NOTE: used by Collection
           // TODO: to be replaced when connection of device gets thread support
           bool succeeded() const { return m_success; }

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
           virtual void parseTracks() = 0;// NOTE: used by Collection
           virtual void writeDatabase() = 0;// NOTE: used by Collection
           #if 0
           void updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track, Itdb_Track *existingMediaDeviceTrack );// NOTE: used by Collection




        signals:
           void copyTracksDone( bool success ); // NOTE: used by Collection
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

           /**
            * Pulls out meta information (e.g. artist string)
            * from ipodtrack, inserts into appropriate map
            * (e.g. ArtistMap).  Sets track's meta info
            * (e.g. artist string) to that extracted from
            * ipodtrack's.
            * @param ipodtrack - track being read from
            * @param track - track being written to
            * @param Map - map where meta information is
            * associated to appropriate meta pointer
            * (e.g. QString artist, ArtistPtr )
            */

           void setupArtistMap( Itdb_Track *ipodtrack, Meta::MediaDeviceTrackPtr track, ArtistMap &artistMap );
           void setupAlbumMap( Itdb_Track *ipodtrack, Meta::MediaDeviceTrackPtr track, AlbumMap &albumMap );
           void setupGenreMap( Itdb_Track *ipodtrack, Meta::MediaDeviceTrackPtr track, GenreMap &genreMap );
           void setupComposerMap( Itdb_Track *ipodtrack, Meta::MediaDeviceTrackPtr track, ComposerMap &composerMap );
           void setupYearMap( Itdb_Track *ipodtrack, Meta::MediaDeviceTrackPtr track, YearMap &yearMap );

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

           /* Collection Variables */

           // Associated collection
           MediaDeviceCollection   *m_memColl;
           // Map of titles, used to check for duplicate tracks
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

        private:
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

#endif
