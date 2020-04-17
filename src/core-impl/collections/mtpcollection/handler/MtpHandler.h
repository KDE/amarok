/****************************************************************************************
 * Copyright (c) 2006 Andy Kelk <andy@mopoke.co.uk>                                     *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef MTPHANDLER_H
#define MTPHANDLER_H

#include <libmtp.h>

#include "MtpPlaylistCapability.h"
#include "MtpReadCapability.h"
#include "MtpWriteCapability.h"

#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include <KIO/Job>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Queue>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>
#include <QSet>
#include <QTemporaryDir>
#include <QTemporaryFile>


class QString;
class QMutex;
class QStringList;

namespace Collections {
    class MtpCollection;
}

namespace Meta
{
typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;
class WorkerThread;

/* The libmtp backend for all Mtp calls */
class MtpHandler : public MediaDeviceHandler
{
        Q_OBJECT

    public:
        explicit MtpHandler( Collections::MtpCollection *mc );
        ~MtpHandler() override;

        friend class WorkerThread;

        void init() override; // collection
        bool isWritable() const override;

        void getCopyableUrls( const Meta::TrackList &tracks ) override;

        QString prettyName() const override;

        void prepareToPlay( Meta::MediaDeviceTrackPtr &track ) override;

        /// Capability-related methods

        bool hasCapabilityInterface( Handler::Capability::Type type ) const override;
        Handler::Capability* createCapabilityInterface( Handler::Capability::Type type ) override;

        friend class Handler::MtpPlaylistCapability;
        friend class Handler::MtpReadCapability;
        friend class Handler::MtpWriteCapability;

    protected:
        /* Parsing of Tracks on Device */
        virtual void prepareToParseTracks();
        virtual bool isEndOfParseTracksList();
        virtual void prepareToParseNextTrack();
        virtual void nextTrackToParse();

        virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track );

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
        virtual void setAssociatePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist );
        virtual void libSavePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist, const QString& name );
        virtual void deletePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist );
        virtual void renamePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist );

        virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) }
        virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track ) { Q_UNUSED( track ) }
        virtual void setDatabaseChanged();

        virtual QString libGetTitle( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetAlbumArtist( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track );
        virtual qint64  libGetLength( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComment( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetBitrate( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetSamplerate( const Meta::MediaDeviceTrackPtr &track );
        virtual qreal   libGetBpm( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetFileSize( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetPlayCount( const Meta::MediaDeviceTrackPtr &track );
        virtual QDateTime libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetRating( const Meta::MediaDeviceTrackPtr &track ) ;
        virtual QString libGetType( const Meta::MediaDeviceTrackPtr &track );
        virtual QUrl    libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track );

        virtual float usedCapacity() const;
        virtual float totalCapacity() const;

        virtual void libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title );
        virtual void libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album );
        virtual void libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist );
        virtual void libSetAlbumArtist( Meta::MediaDeviceTrackPtr &track, const QString& albumArtist );
        virtual void libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer );
        virtual void libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre );
        virtual void libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year );
        virtual void libSetLength( Meta::MediaDeviceTrackPtr &track, int length );
        virtual void libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum );
        virtual void libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment );
        virtual void libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum );
        virtual void libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate );
        virtual void libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate );
        virtual void libSetBpm( Meta::MediaDeviceTrackPtr &track, qreal bpm );
        virtual void libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize );
        virtual void libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount );
        virtual void libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, const QDateTime &lastplayed );
        virtual void libSetRating( Meta::MediaDeviceTrackPtr &track, int rating ) ;
        virtual void libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type );
        virtual void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

        virtual void prepareToCopy() {}
        virtual void prepareToDelete() {}

        /// libmtp-specific
    private Q_SLOTS:
        void slotDeviceMatchSucceeded( ThreadWeaver::JobPointer job );
        void slotDeviceMatchFailed( ThreadWeaver::JobPointer job );

    private:
        bool iterateRawDevices( int numrawdevices, LIBMTP_raw_device_t* rawdevices );
        void getDeviceInfo();

        void terminate();

        int getTrackToFile( const uint32_t id, const QString & filename );

        // Some internal stuff that must be public due to libmtp being in C
        static int progressCallback( uint64_t const sent, uint64_t const total, void const * const data );

        // file-copying related functions
        uint32_t checkFolderStructure( const Meta::TrackPtr track, bool create );
        uint32_t getDefaultParentId( void );
        uint32_t folderNameToID( char *name, LIBMTP_folder_t *folderlist );
        uint32_t subfolderNameToID( const char *name, LIBMTP_folder_t *folderlist, uint32_t parent_id );
        uint32_t createFolder( const char *name, uint32_t parent_id );
        void updateFolders( void );

        QString setTempFile( Meta::MediaDeviceTrackPtr &track, const QString &format );

        virtual void updateTrack( Meta::MediaDeviceTrackPtr &track );

        // mtp database
        LIBMTP_mtpdevice_t      *m_device;

        float                    m_capacity;

        QMap<int, QString>       mtpFileTypes;

        uint32_t                m_default_parent_folder;
        LIBMTP_folder_t        *m_folders;
        QString                 m_folderStructure;
        QString                 m_format;
        QString                 m_name;
        QStringList             m_supportedFiles;

        QMutex                  m_critical_mutex;

        // KIO-related Vars (to be moved elsewhere eventually)
        bool m_isCanceled;
        bool m_wait;
        bool m_dbChanged;

        LIBMTP_track_t* m_currentTrackList;
        LIBMTP_track_t* m_currentTrack;
        LIBMTP_playlist_t* m_currentPlaylistList;
        LIBMTP_playlist_t* m_currentPlaylist;

        QHash<Playlists::MediaDevicePlaylistPtr, LIBMTP_playlist_t*> m_mtpPlaylisthash;

        uint32_t m_trackcounter;

        // Hash that associates an LIBMTP_track_t* to every Track*

        QHash<Meta::MediaDeviceTrackPtr, LIBMTP_track_t*> m_mtpTrackHash;

        // Keeps track of which tracks have been copied/cached for playing

        QHash<Meta::MediaDeviceTrackPtr, QTemporaryFile*> m_cachedTracks;

        // Maps id's to tracks

        QHash<uint32_t, LIBMTP_track_t*> m_idTrackHash;

        // parentid calculated for new track copied to device

        uint32_t m_copyParentId;

        // Used as temporary location for copying files from mtp

        QTemporaryDir *m_tempDir;
};

class WorkerThread : public QObject, public ThreadWeaver::Job
{
        Q_OBJECT

    public:
        WorkerThread( int numrawdevices, LIBMTP_raw_device_t* rawdevices, MtpHandler* handler );
        ~WorkerThread() override;

        bool success() const override;

    protected:
        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

    private:
        bool m_success;
        int m_numrawdevices;
        LIBMTP_raw_device_t* m_rawdevices;
        MtpHandler *m_handler;
};

}
#endif
