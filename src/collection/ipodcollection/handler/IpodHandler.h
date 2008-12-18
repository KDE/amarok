/***************************************************************************
 * Ported to Collection Framework: *
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com> 

 * Original Work: *
 * copyright            : (C) 2005, 2006 by Martin Aumueller <aumuell@reserv.at>
 * copyright            : (C) 2004 by Christian Muehlhaeuser <chris@chris.de>
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

#ifndef IPODHANDLER_H
#define IPODHANDLER_H

/* CMake check for GDK */
#include "config-gdk.h"

extern "C" {
  #include <gpod/itdb.h>
}

#include "Meta.h"
#include "MemoryCollection.h"
#include "IpodMeta.h"
#include "../../../statusbar/StatusBar.h"

#include "kjob.h"
#include <KTempDir>

#include <QObject>
#include <QMap>
#include <QMultiMap>

class QString;
class QFile;
class QDateTime;
class QMutex;

class IpodCollection;

namespace Ipod
{

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

/* The libgpod backend for all Ipod calls */
    class IpodHandler : public QObject, public Meta::Observer
    {
        Q_OBJECT

        public:
           /**
            * Constructor
            */
           IpodHandler( IpodCollection *mc, const QString& mountPoint, QObject* parent );
           /**
            * Destructor
            */
           ~IpodHandler();

           /**
            * Successfully read Ipod database
            */
           bool succeeded() const { return m_success; }

           // Observer Methods

           /** These methods are called when the metadata of a track has changed. */
           virtual void metadataChanged( Meta::TrackPtr track );
           virtual void metadataChanged( Meta::ArtistPtr artist );
           virtual void metadataChanged( Meta::AlbumPtr album );
           virtual void metadataChanged( Meta::GenrePtr genre );
           virtual void metadataChanged( Meta::ComposerPtr composer );
           virtual void metadataChanged( Meta::YearPtr year );

           void detectModel();

           QString itunesDir( const QString &path = QString() ) const;
           QString mountPoint() const { return m_mountPoint; }
           bool openDevice( bool silent=false );
           void copyTrackToDevice( const Meta::TrackPtr &track );
           void copyTrackListToDevice( const Meta::TrackList tracklist );
           bool deleteTrackFromDevice( const Meta::IpodTrackPtr &track );
           void deleteTracksFromDevice( const Meta::TrackList &tracks );
           bool kioCopyTrack( const KUrl &src, const KUrl &dst );
           void deleteFile( const KUrl &url );


           void insertTrackIntoDB( const KUrl &url, const Meta::TrackPtr &track );
           void updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track, Itdb_Track *existingIpodTrack );
           void addTrackInDB( Itdb_Track *ipodtrack );
           bool removeDBTrack( Itdb_Track *track );
           QString           ipodPath( const QString &realPath );
           KUrl determineURLOnDevice( const Meta::TrackPtr &track );
           void parseTracks();
           void addIpodTrackToCollection( Itdb_Track *ipodtrack );
           void getBasicIpodTrackInfo( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track );
           #if GDK_FOUND
           void getCoverArt( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track );
           #endif
           void setCoverArt( Itdb_Track *ipodtrack, const QPixmap &image );
           void setRating( const int newrating );
           void setMountPoint( const QString &mp) { m_mountPoint = mp; }
           QString realPath( const char *ipodPath );
           bool pathExists( const QString &ipodPath, QString *realPath=0 );
           bool writeITunesDB( bool threaded=true );

       // convenience methods to avoid repetitive code

       void setupArtistMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, ArtistMap &artistMap );
       void setupAlbumMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, AlbumMap &albumMap );
       void setupGenreMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, GenreMap &genreMap );
       void setupComposerMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, ComposerMap &composerMap );
       void setupYearMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, YearMap &yearMap );

        public slots:
	    bool initializeIpod();
	void fileTransferred( KJob *job );
    void fileDeleted( KJob *job );

        signals:

        void copyTracksDone();
        void deleteTracksDone();
        void incrementProgress();
        void endProgressOperation( const QObject *owner );

        private slots:

        //void slotCopyTrackToDevice( QString collectionId, Meta::TrackList tracklist );
        


//        void slotQueueTrackForCopy( QString collectionId, Meta::TrackList tracklist );

        private:

        /* Copy/Delete Functions */

        void copyTracksToDevice();

        void copyNextTrackToDevice();
        void deleteNextTrackFromDevice();

        void privateCopyTrackToDevice( const Meta::TrackPtr &track );
        void privateDeleteTrackFromDevice( const Meta::TrackPtr &track );

        /* Copy/Delete Variables */

        Meta::TrackList m_tracksToCopy;
        Meta::TrackList m_tracksToDelete;

        IpodCollection *m_memColl;
        TitleMap m_titlemap;

        ProgressBarNG *m_statusbar;

        // libgpod variables
        Itdb_iTunesDB    *m_itdb;
        Itdb_Device      *m_device;
        Itdb_Playlist    *m_masterPlaylist;

        // tempdir for covers
        KTempDir *m_tempdir;

        /* Ipod Model */
        bool              m_isShuffle;
        bool              m_isMobile;
        bool              m_isIPhone;

        /* Properties of Ipod */
        bool              m_supportsArtwork;
        bool              m_supportsVideo;
        bool              m_rockboxFirmware;
        bool              m_needsFirewireGuid;

        /* Ipod Connection */

        bool              m_autoConnect;
        QString           m_mountPoint;
        QString           m_name;

        /* Success/Failure */

        bool m_dbChanged;
        bool m_copyFailed;
        bool m_isCanceled;
        bool m_wait;
        // whether Itdb_Track is created correctly
        bool m_trackCreated;
        // whether read Ipod DB or not
        bool m_success;

        // TODO: Implement lockfile
        // QFile *m_lockFile;

        // TODO: Implement podcasts
        // podcasts
        // Itdb_Playlist* m_podcastPlaylist;


    };
}
#endif
