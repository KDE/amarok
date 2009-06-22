/***************************************************************************
 * Ported to Collection Framework: *
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

 * Original Work: *
 * copyright            : (C) 2006 Andy Kelk <andy@mopoke.co.uk>
 *
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

/* The libmtp backend for all Mtp calls */
class MtpHandler : public MediaDeviceHandler
{

#if 0






    // file io functions
    bool kioCopyTrack( const KUrl &src, const KUrl &dst );
    void deleteFile( const KUrl &url );



    // internal mtp functions

    int                     readMtpMusic( void );
    void getBasicMtpTrackInfo( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track );
    void setBasicMtpTrackInfo( LIBMTP_track_t *trackmeta, Meta::MtpTrackPtr track );

    QString getFormat( LIBMTP_track_t *mtptrack );

    // miscellaneous internal functions
    void addMtpTrackToCollection( LIBMTP_track_t *mtptrack );

    // convenience methods to avoid repetitive code

    void setupArtistMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, ArtistMap &artistMap );
    void setupAlbumMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, AlbumMap &albumMap );
    void setupGenreMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, GenreMap &genreMap );
    void setupComposerMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, ComposerMap &composerMap );
    void setupYearMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, YearMap &yearMap );

public slots:
    void fileTransferred( KJob *job );
    void fileDeleted( KJob *job );

signals:
    void succeeded();
    void failed();

    void copyTracksDone( bool success );
    void deleteTracksDone();

    void setProgress( int steps );
    void incrementProgress();
    void endProgressOperation( const QObject *owner );



    void slotCopyNextTrackFailed( ThreadWeaver::Job* job );
    void slotCopyNextTrackToDevice( ThreadWeaver::Job* job );

    void copyNextTrackToDevice();


#endif
    Q_OBJECT
public:
    MtpHandler( MtpCollection *mc );
    virtual ~MtpHandler();

    virtual bool isWritable() const;

    // mtp-specific functions

    // connection
    void init();
    bool iterateRawDevices( int numrawdevices, LIBMTP_raw_device_t* rawdevices );
    void getDeviceInfo();

    void terminate();

    // information-gathering

    virtual void getCopyableUrls( const Meta::TrackList &tracks );

    int getTrackToFile( const uint32_t id, const QString & filename );
    virtual QString prettyName() const;

/*
    QMap<Meta::TrackPtr, QString> tracksFailed() const
    {
        return m_tracksFailed;
    }
*/
    // Some internal stuff that must be public due to libmtp being in C

    static int progressCallback( uint64_t const sent, uint64_t const total, void const * const data );
    // Progress Bar functions

private:
    // file-copying related functions
    uint32_t checkFolderStructure( const Meta::TrackPtr track, bool create );
    uint32_t getDefaultParentId( void );
    uint32_t folderNameToID( char *name, LIBMTP_folder_t *folderlist );
    uint32_t subfolderNameToID( const char *name, LIBMTP_folder_t *folderlist, uint32_t parent_id );
    uint32_t createFolder( const char *name, uint32_t parent_id );
    void updateFolders( void );

    public:

    

               /// Methods that wrap get/set of information using libmtp

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
           virtual int     libGetRating( const Meta::MediaDeviceTrackPtr &track );
           virtual QString libGetType( const Meta::MediaDeviceTrackPtr &track );
           virtual QString libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track );

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
           virtual void    libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed);
           virtual void    libSetRating( Meta::MediaDeviceTrackPtr &track, int rating );
           virtual void    libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type );
           virtual void    libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

           //void setCoverArt( Itdb_Track *ipodtrack, const QPixmap &image );

           /// Create new track

           virtual void libCreateTrack(const Meta::MediaDeviceTrackPtr& track );
           void findPathToCopyMtp( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack );
           virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack );
           //virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track );

           //virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track ) {};
           virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track );
           //virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track );

           virtual void databaseChanged();

           /// Parse iteration methods

           virtual void prepareToParse();
           virtual bool isEndOfParseList();
           virtual void prepareToParseNextTrack();
           virtual void nextTrackToParse();
           virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track );

           virtual QStringList supportedFormats();

           virtual void prepareToPlay( Meta::MediaDeviceTrackPtr &track );
           QString setTempFile( Meta::MediaDeviceTrackPtr &track, const QString &format );

           // libmtp-specific

           


           private slots:
    void slotDeviceMatchSucceeded( ThreadWeaver::Job* job );
    void slotDeviceMatchFailed( ThreadWeaver::Job* job );

    private:
/*
    Meta::TrackList m_tracksToCopy;
    Meta::TrackList m_tracksToDelete;

    Meta::TrackPtr m_lastTrackCopied;
    QMap<Meta::TrackPtr, QString> m_tracksFailed;
*/
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


    bool m_success; // tells if connecting worked or not
    bool             m_trackCreated;

    // KIO-related Vars (to be moved elsewhere eventually)
/*
    bool m_copyFailed;
*/
    bool m_isCanceled;
    bool m_wait;
    bool m_dbChanged;
    LIBMTP_track_t *m_currtracklist;
    
    LIBMTP_track_t *m_currtrack;

    // Hash that associates an LIBMTP_track_t* to every Track*

    QHash<Meta::MediaDeviceTrackPtr, LIBMTP_track_t*> m_mtptrackhash;

    // Keeps track of which tracks have been copied/cached for playing

    QHash<Meta::MediaDeviceTrackPtr, KTemporaryFile*> m_cachedtracks;

    // Used as temporary location for copying files from mtp

    KTempDir m_tempdir;

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
/*
class CopyWorkerThread : public ThreadWeaver::Job
{
    Q_OBJECT
public:
    CopyWorkerThread( const Meta::TrackPtr &track, MtpHandler* handler );
    virtual ~CopyWorkerThread();

    virtual bool success() const;

protected:
    virtual void run();

private:
    bool m_success;
    Meta::TrackPtr m_track;
    MtpHandler *m_handler;
};
*/
}
#endif
