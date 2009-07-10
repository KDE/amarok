/****************************************************************************************
 * Copyright (c) 2006 Andy Kelk <andy@mopoke.co.uk>                                     *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef MTPHANDLER_H
#define MTPHANDLER_H

#include <libmtp.h>

#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include <KIO/Job>
#include <KTempDir>
#include <KTemporaryFile>
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>
#include <QSet>

class QString;
class QMutex;
class QStringList;

class MtpCollection;

namespace Meta
{
typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;
class WorkerThread;

/* The libmtp backend for all Mtp calls */
class MtpHandler : public MediaDeviceHandler
{
    Q_OBJECT
public:
    MtpHandler( MtpCollection *mc );
    virtual ~MtpHandler();

    friend class WorkerThread;

    virtual void init(); // collection
    virtual bool isWritable() const;
    virtual void writeDatabase() {}

    virtual void getCopyableUrls( const Meta::TrackList &tracks );

    virtual QString prettyName() const;

    virtual void prepareToPlay( Meta::MediaDeviceTrackPtr &track );

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
    virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
    {
        Q_UNUSED( track ) return true;
    }
    virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track );
    virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track );

    virtual Meta::MediaDeviceTrackPtr libGetTrackPtrForTrackStruct();

    virtual QString libGetPlaylistName();

    virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track )
    {
        Q_UNUSED( track )
    }
    virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track )
    {
        Q_UNUSED( track )
    }
    virtual void databaseChanged();

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
    virtual KUrl    libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track );

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

    virtual void prepareToCopy() {}
    virtual void prepareToDelete() {}


    /// libmtp-specific


private slots:
    void slotDeviceMatchSucceeded( ThreadWeaver::Job* job );
    void slotDeviceMatchFailed( ThreadWeaver::Job* job );

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

    QMap<int, QString>       mtpFileTypes;

    uint32_t                m_default_parent_folder;
    LIBMTP_folder_t        *m_folders;
    QString                 m_folderStructure;
    QString                 m_format;
    QString                 m_name;
    QStringList             m_supportedFiles;

    QMutex                  m_critical_mutex;

    // KIO-related Vars (to be moved elsewhere eventually)
    /*
        bool m_copyFailed;
    */
    bool m_isCanceled;
    bool m_wait;
    bool m_dbChanged;
    LIBMTP_track_t *m_currtracklist;

    LIBMTP_track_t *m_currtrack;

    LIBMTP_playlist_t *m_currplaylistlist;
    LIBMTP_playlist_t *m_currplaylist;

    uint32_t m_trackcounter;

    // Hash that associates an LIBMTP_track_t* to every Track*

    QHash<Meta::MediaDeviceTrackPtr, LIBMTP_track_t*> m_mtptrackhash;

    // Keeps track of which tracks have been copied/cached for playing

    QHash<Meta::MediaDeviceTrackPtr, KTemporaryFile*> m_cachedtracks;

    // Maps id's to tracks

    QHash<uint32_t, LIBMTP_track_t*> m_idtrackhash;

    // Used as temporary location for copying files from mtp

    KTempDir *m_tempdir;

};

class WorkerThread : public ThreadWeaver::Job
{
    Q_OBJECT
public:
    WorkerThread( int numrawdevices, LIBMTP_raw_device_t* rawdevices, MtpHandler* handler );
    virtual ~WorkerThread();

    virtual bool success() const;

protected:
    virtual void run();

private:

    bool m_success;
    int m_numrawdevices;
    LIBMTP_raw_device_t* m_rawdevices;
//    QString m_serial;
    MtpHandler *m_handler;


};

}
#endif
