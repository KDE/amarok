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

#include "Meta.h"
#include "MemoryCollection.h"
#include "MtpMeta.h"
#include "../../../statusbar/StatusBar.h"

#include "kjob.h"
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>



class QString;
class QFile;
class QDateTime;
class QMutex;
class QStringList;

class MtpCollection;

namespace Mtp
{

    typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

/* The libmtp backend for all Mtp calls */
    class MtpHandler : public QObject
    {

        Q_OBJECT

        public:
            MtpHandler( MtpCollection *mc, QObject* parent );
           ~MtpHandler();

           void init( const QString &serial ); // called by collection
           void getDeviceInfo();

           // thread-related functions
           bool iterateRawDevices( int numrawdevices, LIBMTP_raw_device_t* rawdevices, const QString &serial );

       // external functions
       void copyTrackListToDevice( const Meta::TrackList tracklist );
//       bool deleteTrackFromDevice( const Meta::MtpTrackPtr &track );
       void deleteTracksFromDevice( const Meta::TrackList &tracks );
       int getTrackToFile( const uint32_t id, const QString & filename );
       void parseTracks();
       void updateTrackInDB( const Meta::MtpTrackPtr track );
       QString prettyName() const;
       void terminate();
       bool succeeded() const { return m_success; }

       // Some internal stuff that must be public due to libmtp being in C

       static int progressCallback( uint64_t const sent, uint64_t const total, void const * const data );
       // Progress Bar functions

       void setBarMaximum( int total );
       void setBarProgress( int steps );
       void endBarProgressOperation();

        private:
            // file-copying related functions
            uint32_t checkFolderStructure( const Meta::TrackPtr track, bool create );
            uint32_t getDefaultParentId( void );
            uint32_t folderNameToID( char *name, LIBMTP_folder_t *folderlist );
            uint32_t subfolderNameToID( const char *name, LIBMTP_folder_t *folderlist, uint32_t parent_id );
            uint32_t createFolder( const char *name, uint32_t parent_id );
            void updateFolders( void );

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

           void copyTracksDone();
           void deleteTracksDone();

           void setProgress( int steps );
           void incrementProgress();
           void endProgressOperation( const QObject *owner );

        private slots:
            void slotDeviceMatchSucceeded( ThreadWeaver::Job* job);
            void slotDeviceMatchFailed( ThreadWeaver::Job* job);

        private:

            void copyTracksToDevice();
            void copyNextTrackToDevice();
            void privateCopyTrackToDevice( const Meta::TrackPtr &track );
            MtpCollection *m_memColl;

            TitleMap m_titlemap;

            void deleteNextTrackFromDevice();
            void privateDeleteTrackFromDevice( const Meta::MtpTrackPtr &track );
            Meta::TrackList m_tracksToCopy;
            Meta::TrackList m_tracksToDelete;

            ProgressBarNG *m_statusbar;

        // mtp database

        LIBMTP_mtpdevice_t      *m_device;

        QMap<int,QString>       mtpFileTypes;

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

        bool m_copyFailed;
        bool m_isCanceled;
        bool m_wait;


    };

    class WorkerThread : public ThreadWeaver::Job
    {
        Q_OBJECT
        public:
            WorkerThread( int numrawdevices, LIBMTP_raw_device_t* rawdevices, const QString &serial, MtpHandler* handler );
            virtual ~WorkerThread();

            virtual bool success() const;

        protected:
            virtual void run();

        private:
            bool m_success;
            int m_numrawdevices;
            LIBMTP_raw_device_t* m_rawdevices;
            QString m_serial;
            MtpHandler *m_handler;
    };
    
}
#endif
