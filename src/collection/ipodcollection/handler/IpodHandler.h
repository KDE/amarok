/****************************************************************************************
 * Copyright (c) 2005,2006 Martin Aumueller <aumuell@reserv.at>                         *
 * Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>                           *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef IPODHANDLER_H
#define IPODHANDLER_H

// Taglib includes
#include <audioproperties.h>
#include <fileref.h>

/* CMake check for GDK */
#include <config-gdk.h>

extern "C"
{
#include <gpod/itdb.h>
}

#include "IpodArtworkCapability.h"
#include "IpodPlaylistCapability.h"
#include "IpodReadCapability.h"
#include "IpodWriteCapability.h"
#include "IpodDeviceInfo.h"

#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include <KDiskFreeSpaceInfo>
#include <KIO/Job>
#include "kjob.h"
#include <ctime> // for kjob.h
#include <KTempDir>
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>

namespace Solid {
    class StorageAccess;
}

class QString;
class QMutex;

class IpodCollection;

typedef QHash<QString, QString> AttributeHash;
typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

// NOTE: podcasts NYI
/*
struct PodcastInfo
{
    // per show
    QString url;
    QString description;
//    QDateTime date;
    QString author;
    bool listened;

    // per channel
    QString rss;

    PodcastInfo() { listened = false; }
};
*/
namespace Meta
{

    typedef QMap<QString, Meta::TrackPtr> TrackMap;
    typedef QMap<QString, Meta::AlbumPtr> AlbumMap;

/* The libgpod backend for all Ipod calls */
class IpodHandler : public Meta::MediaDeviceHandler
{
    Q_OBJECT

    public:
        IpodHandler( IpodCollection *mc, const IpodDeviceInfo *deviceInfo );
        virtual ~IpodHandler();

        virtual void init(); // collection
        virtual bool isWritable() const;
        virtual bool supportsArtwork() const { return m_supportsArtwork; }

        virtual QString prettyName() const;

        virtual QList<QAction *> collectionActions();

        /// Capability-related methods

        virtual bool hasCapabilityInterface( Handler::Capability::Type type ) const;
        virtual Handler::Capability* createCapabilityInterface( Handler::Capability::Type type );

        friend class Handler::IpodArtworkCapability;
        friend class Handler::IpodPlaylistCapability;
        friend class Handler::IpodReadCapability;
        friend class Handler::IpodWriteCapability;
        friend class StaleWorkerThread;
        friend class OrphanedWorkerThread;
        friend class AddOrphanedWorkerThread;
        friend class SyncArtworkWorkerThread;

        /// Ipod-Specific Methods
        QMap<Meta::TrackPtr, QString> tracksFailed() const { return m_tracksFailed; }
        QString mountPoint() const { return m_deviceInfo->mountPoint(); }
#if 0
        void setMountPoint( const QString &mp ) { m_mountPoint = mp; }
#endif

        // NOTE: do not use writeITunesDB,
        // use the threaded writeDatabase
        bool writeITunesDB( bool threaded = false );

    public slots:
        virtual void writeDatabase();

        void slotInitializeIpod();
        void slotStaleOrphaned();
        void slotSyncArtwork();

    protected:
        /// Functions for PlaylistCapability
        /**
         * Writes to the device's database if it has one, otherwise
         * simply calls slotDatabaseWritten to continue the workflow.
         */
        virtual void prepareToParsePlaylists();
        virtual bool isEndOfParsePlaylistsList();
        virtual void prepareToParseNextPlaylist();
        virtual void nextPlaylistToParse();

        virtual bool shouldNotParseNextPlaylist();

        virtual void prepareToParsePlaylistTracks();
        virtual bool isEndOfParsePlaylist();
        virtual void prepareToParseNextPlaylistTrack();
        virtual void nextPlaylistTrackToParse();

        virtual QStringList supportedFormats();

        virtual void findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack );
        virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack );
        virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track );
        virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track );
        virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track );

        virtual Meta::MediaDeviceTrackPtr libGetTrackPtrForTrackStruct();

        virtual QString libGetPlaylistName();
        void setAssociatePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
        void libSavePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name );
        void deletePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
        void renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );

        virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track );
        virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track );
        virtual void setDatabaseChanged();

        virtual void libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title );
        virtual void libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album );
        virtual void libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist );
        virtual void libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer );
        virtual void libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre );
        virtual void libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year );
        virtual void libSetLength( Meta::MediaDeviceTrackPtr &track, int length );
        virtual void libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum );
        virtual void libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment );
        virtual void libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum );
        virtual void libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate );
        virtual void libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate );
        virtual void libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm );
        virtual void libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize );
        virtual void libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount );
        virtual void libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed );
        virtual void libSetRating( Meta::MediaDeviceTrackPtr &track, int rating ) ;
        virtual void libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type );
        virtual void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

        virtual void libSetCoverArt( Meta::MediaDeviceTrackPtr &track, const QPixmap &image );
        virtual void libSetCoverArtPath( Meta::MediaDeviceTrackPtr &track, const QString &path );

        virtual void prepareToCopy();
        virtual void prepareToDelete();

    private:
        enum FileType
        {
            mp3,
            ogg,
            flac,
            mp4
        };

        /// Functions for ReadCapability
        virtual void prepareToParseTracks();
        virtual bool isEndOfParseTracksList();
        virtual void prepareToParseNextTrack();
        virtual void nextTrackToParse();

        virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track );

        virtual QString libGetTitle( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track );
        virtual qint64  libGetLength( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComment( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetBitrate( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetSamplerate( const Meta::MediaDeviceTrackPtr &track );
        virtual float   libGetBpm( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetFileSize( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetPlayCount( const Meta::MediaDeviceTrackPtr &track );
        virtual uint    libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetRating( const Meta::MediaDeviceTrackPtr &track ) ;
        virtual QString libGetType( const Meta::MediaDeviceTrackPtr &track );
        virtual KUrl    libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track );
        virtual QPixmap libGetCoverArt( const Meta::MediaDeviceTrackPtr &track );

        virtual float usedCapacity() const;
        virtual float totalCapacity() const;

        /// Ipod Methods

        /**
        * Finds tracks that are in the database, but whose file
        * no longer exists or cannot be found
        * @return the list of stale tracks found
        */
        Meta::TrackList staleTracks();

        /**
        * Finds tracks that are in the Music folders, but that
        * do not have an entry in the database tied to it
        * @return list of filepaths of orphaned tracks
        */
        QStringList orphanedTracks();

        bool findStale();
        bool findOrphaned();
        bool addNextOrphaned();

        bool syncArtwork();

        bool initializeIpod();

        bool removeDBTrack( Itdb_Track *track );

        /* libgpod Information Extraction Methods */

        void detectModel();
        bool writeToSysInfoFile( const QString &text );
        bool appendToSysInfoFile( const QString &text );
        bool writeFirewireGuid();
        KUrl determineURLOnDevice( const Meta::TrackPtr &track );
        QString itunesDir( const QString &path = QString() ) const;
        QString ipodPath( const QString &realPath ) const;
        bool pathExists( const QString &ipodPath, QString *realPath = 0 );
        QString realPath( const char *ipodPath );

        /* Cover Art functions */
        QString ipodArtFilename( const Itdb_Track *ipodtrack ) const;

        /* File I/O Methods */
        // TODO: abstract copy/delete methods (not too bad)
        bool kioCopyTrack( const KUrl &src, const KUrl &dst );
        void deleteFile( const KUrl &url );

        /**
        * Handler Variables
        */
        /* Collection Variables */

        // Associated collection
        // Map of titles, used to check for duplicate tracks

        /* libgpod variables */
        Itdb_iTunesDB    *m_itdb;
        Itdb_Playlist    *m_masterPlaylist;
        GList            *m_currtracklist;
        Itdb_Track       *m_currtrack;
        QHash<QString,Itdb_Track*> m_files;
        Meta::TrackList m_staletracks;
        int m_staletracksremoved;
        int m_orphanedadded;
        QStringList m_orphanedPaths;

        // For space checks
        QString               m_filepath;
        float                 m_capacity;

        // for playlist parsing
        GList            *m_currplaylistlist;
        Itdb_Playlist    *m_currplaylist;

        /* Lockers */
        QMutex            m_dbLocker; // DB only written by 1 thread at a time
        QMutex            m_joblocker; // lets only 1 job finish at a time
        int               m_jobcounter; // keeps track of copy jobs present

        /* Copy/Delete Variables */
        Meta::TrackList   m_tracksToDelete;

        QMap<KUrl, Meta::TrackPtr> m_trackscopying; // associates source url to track of source url
        QMap<Meta::TrackPtr, KUrl> m_trackdesturl; // keeps track of destination url for new tracks, mapped from source track

        QMap<KUrl, Meta::TrackPtr> m_tracksdeleting; // associates source url to track of source url being deleted

        Itdb_Track       *m_libtrack;

        /* Ipod Connection */
        bool    m_autoConnect;
        QString m_name;
        const IpodDeviceInfo *m_deviceInfo;

        /* Ipod Model */
        bool m_isShuffle;

        /* Properties of Ipod */
        bool m_supportsArtwork;
        bool m_supportsVideo;
        bool m_rockboxFirmware;
        bool m_needsFirewireGuid;
        mutable QString m_controlDir;

        /* Success/Failure */
        bool m_dbChanged;
        bool m_copyFailed;
        bool m_isCanceled;
        bool m_wait;

        // whether Itdb_Track is created correctly
        bool m_trackCreated;

        /* Miscellaneous Variables */

        // Hash that associates an Itdb_Track* to every Track*
        QHash<Meta::MediaDeviceTrackPtr, Itdb_Track*> m_itdbtrackhash;

        // Hash that associates an Itdb_Playlist* to every PlaylistPtr
        QHash<Meta::MediaDevicePlaylistPtr, Itdb_Playlist*> m_itdbplaylisthash;

        // tracks that failed to copy
        QMap<Meta::TrackPtr, QString> m_tracksFailed;

        // tempdir for covers
        KTempDir *m_tempdir;
        QSet<QString> m_coverArt;

        // TODO: Implement lockfile
        // QFile *m_lockFile;

        // TODO: Implement podcasts
        // podcasts
        // Itdb_Playlist* m_podcastPlaylist;

    private slots:
        void fileTransferred( KJob *job );
        void fileDeleted( KJob *job );

        void slotDBWriteFailed( ThreadWeaver::Job* job );
        void slotDBWriteSucceeded( ThreadWeaver::Job* job );

        void slotStaleFailed( ThreadWeaver::Job* job );
        void slotStaleSucceeded( ThreadWeaver::Job* job );

        void slotOrphanedFailed( ThreadWeaver::Job* job );
        void slotOrphanedSucceeded( ThreadWeaver::Job* job );

        void slotAddOrphanedFailed( ThreadWeaver::Job* job );
        void slotAddOrphanedSucceeded( ThreadWeaver::Job* job );

        void slotSyncArtworkFailed( ThreadWeaver::Job *job );
        void slotSyncArtworkSucceeded( ThreadWeaver::Job *job );

        void slotCopyingDone( KIO::Job* job, KUrl from, KUrl to, time_t mtime, bool directory, bool renamed );

        void slotOrphaned();
};

class AbstractIpodWorkerThread : public ThreadWeaver::Job
{
        Q_OBJECT
    public:
        AbstractIpodWorkerThread( IpodHandler *handler )
            : ThreadWeaver::Job()
            , m_handler( handler )
            , m_success( false )
        {
            connect( this, SIGNAL( done( ThreadWeaver::Job* ) ),
                     this, SLOT( deleteLater() ), Qt::QueuedConnection );
        }

        virtual ~AbstractIpodWorkerThread() {}

        virtual bool success() const { return m_success; }

    protected:
        virtual void run() = 0;

        IpodHandler *m_handler;
        bool m_success;
};


class DBWorkerThread : public AbstractIpodWorkerThread
{
        Q_OBJECT
    public:
        DBWorkerThread( IpodHandler* handler );
        virtual ~DBWorkerThread() {}

    protected:
        virtual void run();
};

class StaleWorkerThread : public AbstractIpodWorkerThread
{
        Q_OBJECT
    public:
        StaleWorkerThread( IpodHandler* handler );
        virtual ~StaleWorkerThread() {}

    protected:
        virtual void run();
};

class OrphanedWorkerThread : public AbstractIpodWorkerThread
{
        Q_OBJECT
    public:
        OrphanedWorkerThread( IpodHandler* handler );
        virtual ~OrphanedWorkerThread() {}

    protected:
        virtual void run();
};

class AddOrphanedWorkerThread : public AbstractIpodWorkerThread
{
        Q_OBJECT
    public:
        AddOrphanedWorkerThread( IpodHandler* handler );
        virtual ~AddOrphanedWorkerThread() {}

    protected:
        virtual void run();
};

class SyncArtworkWorkerThread : public AbstractIpodWorkerThread
{
        Q_OBJECT
    public:
        SyncArtworkWorkerThread( IpodHandler* handler );
        virtual ~SyncArtworkWorkerThread() {}

    protected:
        virtual void run();
};

}
#endif
