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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef IPODHANDLER_H
#define IPODHANDLER_H

/* CMake check for GDK */
#include <config-gdk.h>

extern "C"
{
#include <gpod/itdb.h>
}

#include "IpodPlaylistCapability.h"
#include "IpodReadCapability.h"

#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include "mediadevicecollection_export.h"

#include <KIO/Job>
#include "kjob.h"
#include <ctime> // for kjob.h
#include <KTempDir>
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>

class QString;
class QMutex;

class IpodCollection;


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

/* The libgpod backend for all Ipod calls */
class MEDIADEVICECOLLECTION_EXPORT IpodHandler : public Meta::MediaDeviceHandler
{
    Q_OBJECT

public:
    IpodHandler( IpodCollection *mc, const QString& mountPoint );
    virtual ~IpodHandler();

    virtual void init(); // collection
    virtual bool isWritable() const;
    virtual void writeDatabase();

    virtual QString prettyName() const;

    virtual QList<PopupDropperAction *> collectionActions();

    /// Capability-related methods

    virtual bool hasCapabilityInterface( Handler::Capability::Type type ) const;
    virtual Handler::Capability* createCapabilityInterface( Handler::Capability::Type type );

    friend class Handler::IpodPlaylistCapability;
    friend class Handler::IpodReadCapability;

    public slots:
        void slotInitializeIpod();

    protected:

    /// Functions for PlaylistCapability

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

    virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track );
    virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track );
    virtual void databaseChanged();

    virtual void    libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title );
    virtual void    libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album );
    virtual void    libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist );
    virtual void    libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer );
    virtual void    libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre );
    virtual void    libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year );
    virtual void    libSetLength( Meta::MediaDeviceTrackPtr &track, int length );
    virtual void    libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum );
    virtual void    libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment );
    virtual void    libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum );
    virtual void    libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate );
    virtual void    libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate );
    virtual void    libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm );
    virtual void    libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize );
    virtual void    libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount );
    virtual void    libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed );
    virtual void    libSetRating( Meta::MediaDeviceTrackPtr &track, int rating ) ;
    virtual void    libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type );
    virtual void    libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

    virtual void prepareToCopy();
    virtual void prepareToDelete();

    /// Ipod-Specific Methods

public:

    /* Set Methods */

    QMap<Meta::TrackPtr, QString> tracksFailed() const
    {
        return m_tracksFailed;
    }

    QString mountPoint() const
    {
        return m_mountPoint;
    }

    void setMountPoint( const QString &mp )
    {
        m_mountPoint = mp;
    }

    /* Methods Provided for Collection */

#if 0
    virtual void copyTrackListToDevice( const Meta::TrackList tracklist );
    void deleteTrackListFromDevice( const Meta::TrackList &tracks );
    /**
     * Parses Ipod DB and creates a Meta::IpodTrack
     * for each track in the DB
     */
    //virtual void parseTracks();
    void updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track, Itdb_Track *existingIpodTrack );
#endif

    // NOTE: do not use writeITunesDB,
    // use the threaded writeDatabase
    bool writeITunesDB( bool threaded = false );

public slots:
    bool initializeIpod();


private:

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
    virtual int     libGetLength( const Meta::MediaDeviceTrackPtr &track );
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
    virtual QString libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track );

    /// Ipod Methods

    bool removeDBTrack( Itdb_Track *track );

    /* libgpod Information Extraction Methods */

    void detectModel();
    bool writeToSysInfoFile( const QString &text );
    bool appendToSysInfoFile( const QString &text );
    bool writeFirewireGuid();
    KUrl determineURLOnDevice( const Meta::TrackPtr &track );
    QString itunesDir( const QString &path = QString() ) const;
    QString ipodPath( const QString &realPath );
    bool pathExists( const QString &ipodPath, QString *realPath = 0 );
    QString realPath( const char *ipodPath );

    /* Cover Art functions */
    /*
    QString ipodArtFilename( const Itdb_Track *ipodtrack ) const;
    void getCoverArt( const Itdb_Track *ipodtrack );
    */
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
    bool              m_autoConnect;
    QString           m_mountPoint;
    QString           m_name;

    /* Ipod Model */
    bool              m_isShuffle;
    bool              m_isMobile;
    bool              m_isIPhone;

    /* Properties of Ipod */
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
    // whether read Ipod DB or not
    //bool m_success;

    /* Miscellaneous Variables */

    // Hash that associates an Itdb_Track* to every Track*

    QHash<Meta::MediaDeviceTrackPtr, Itdb_Track*> m_itdbtrackhash;

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

    void slotCopyingDone( KIO::Job* job, KUrl from, KUrl to, time_t mtime, bool directory, bool renamed );
};

class DBWorkerThread : public ThreadWeaver::Job
{
    Q_OBJECT
public:
    DBWorkerThread( IpodHandler* handler );
    virtual ~DBWorkerThread();

    virtual bool success() const;

protected:
    virtual void run();

private:
    bool m_success;
    IpodHandler *m_handler;
};
}
#endif
