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

#ifndef UMSHANDLER_H
#define UMSHANDLER_H

// Taglib includes
//#include <audioproperties.h>
//#include <fileref.h>

//#include "UmsArtworkCapability.h"
//#include "UmsPlaylistCapability.h"
#include "UmsReadCapability.h"
#include "UmsWriteCapability.h"

#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include "mediadevicecollection_export.h"

#include <KDiskFreeSpaceInfo>

#include <KDirWatch>
#include <KIO/Job>
#include "kjob.h"
#include <ctime> // for kjob.h
#include <KTempDir>
#include <threadweaver/Job.h>

#include <QList>
#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>

namespace Solid {
    class StorageAccess;
}

class UmsCollection;

class KDirLister;
class KFileItem;
class KUrl;

class UmsPodcastProvider;

class QAction;
class QString;
class QMutex;

typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

namespace Meta
{

    typedef QMap<QString, Meta::TrackPtr> TrackMap;

/* The backend for all Ums calls */
class UmsHandler : public Meta::MediaDeviceHandler
{
    Q_OBJECT

    public:
        UmsHandler( UmsCollection *mc, const QString& mountPoint );
        virtual ~UmsHandler();

        virtual void init(); // collection
        virtual QString baseMusicFolder() const;
        virtual bool isWritable() const;
        virtual bool isOrganizable() const;

        virtual QString prettyName() const;

        virtual QList<QAction *> collectionActions();

        /// Capability-related methods

        virtual bool hasCapabilityInterface( Handler::Capability::Type type ) const;
        virtual Handler::Capability* createCapabilityInterface( Handler::Capability::Type type );

        friend class Handler::UmsReadCapability;
        friend class Handler::UmsWriteCapability;

        /// Ums-Specific Methods
        QMap<Meta::TrackPtr, QString> tracksFailed() const { return m_tracksFailed; }
        QString mountPoint() const { return m_mountPoint; }
        void setMountPoint( const QString &mp ) { m_mountPoint = mp; }

        QStringList mimetypes() { return m_mimetypes; }

    public slots:

    protected:
        /// Functions for PlaylistCapability
        /**
         * Writes to the device's database if it has one, otherwise
         * simply calls slotDatabaseWritten to continue the workflow.
         */
#if 0
        virtual void prepareToParsePlaylists();
        virtual bool isEndOfParsePlaylistsList();
        virtual void prepareToParseNextPlaylist();
        virtual void nextPlaylistToParse();

        virtual bool shouldNotParseNextPlaylist();

        virtual void prepareToParsePlaylistTracks();
        virtual bool isEndOfParsePlaylist();
        virtual void prepareToParseNextPlaylistTrack();
        virtual void nextPlaylistTrackToParse();
#endif
        virtual QStringList supportedFormats();

        virtual void findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack );
        virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack );
        virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track );

        virtual void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

        virtual void prepareToCopy();
        virtual void prepareToDelete();

        virtual void updateTrack( Meta::MediaDeviceTrackPtr &track );

        virtual void endTrackRemove();
#if 0

        virtual QString libGetPlaylistName();
        void setAssociatePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
        void libSavePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name );
        void deletePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
        void renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );

        // TODO: MediaDeviceTrackPtr
        virtual void libSetCoverArt( Itdb_Track *umstrack, const QPixmap &image );
        virtual void setCoverArt( Itdb_Track *umstrack, const QString &path );
#endif
    private slots:
        void slotConfigure();

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

        Meta::TrackPtr sourceTrack();

        virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track );

        int addPath( const QString &path );

        virtual float usedCapacity() const;
        virtual float totalCapacity() const;

        /// Ums Methods
        /* File I/O Methods */
        bool kioCopyTrack( const KUrl &src, const KUrl &dst );
        void deleteFile( const KUrl &url );

        /**
        * Handler Variables
        */

        KDirWatch             m_watcher;

        QStringList           m_currtracklist;
        int                   m_listpos; // list position
        Meta::TrackPtr        m_currtrack;
        Meta::MediaDeviceTrackPtr m_destTrack;

        QList<QString>        m_dirtylist;

        QStringList           m_dirList;
        int                   m_dirs;

        QTimer                m_timer;
        QTimer                m_dirtytimer;

        QStringList           m_formats;
        QStringList           m_mimetypes;

        // For space checks
        QString               m_filepath;
        float                 m_capacity;

        /* Lockers */
        QMutex            m_joblocker; // lets only 1 job finish at a time
        int               m_jobcounter; // keeps track of copy jobs present

        /* Copy/Delete Variables */
        Meta::TrackList   m_tracksToDelete;

        QMap<KUrl, Meta::TrackPtr> m_trackscopying; // associates source url to track of source url
        QMap<Meta::TrackPtr, KUrl> m_trackdesturl; // keeps track of destination url for new tracks, mapped from source track
        QMap<Meta::TrackPtr, Meta::MediaDeviceTrackPtr> m_srctodest;
        QMap<KUrl, Meta::TrackPtr> m_tracksdeleting; // associates source url to track of source url being deleted

        /* Ums Connection */
        bool    m_autoConnect;
        QString m_mountPoint;
        bool    m_wasMounted;
        QString m_name;

        /* Ums Parsing */

        bool m_parsed;
        QAction *m_parseAction;

        /* Success/Failure */
        bool m_dbChanged;
        bool m_copyFailed;
        bool m_isCanceled;
        bool m_wait;

        /* Miscellaneous Variables */

        // Hash that associates an MetaFile::Track to every Track*
        QHash<Meta::MediaDeviceTrackPtr, Meta::TrackPtr> m_umstrackhash;

        QHash<QString,Meta::MediaDeviceTrackPtr> m_files; // path, MDTrackPtr

        // Hash that associates an Itdb_Playlist* to every PlaylistPtr
        //QHash<Meta::MediaDevicePlaylistPtr, Itdb_Playlist*> m_itdbplaylisthash;

        // tracks that failed to copy
        QMap<Meta::TrackPtr, QString> m_tracksFailed;

        // tempdir for covers
        KTempDir *m_tempdir;
        QSet<QString> m_coverArt;

        //direct implementation of a podcast provider NOT using the MD::Capabilities
        UmsPodcastProvider *m_podcastProvider;
        QAction *m_configureAction;
        QString m_podcastPath;

    private slots:
        void slotCreateEntry( const QString &path );
        void slotCheckDirty();
        void slotDirtyEntry( const QString &path );
        void slotDeleteEntry( const QString &path );

        void fileTransferred( KJob *job );
        void fileDeleted( KJob *job );

        void slotCopyingDone( KIO::Job* job, KUrl from, KUrl to, time_t mtime, bool directory, bool renamed );

};


}
#endif
