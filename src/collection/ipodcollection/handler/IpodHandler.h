/***************************************************************************
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com> 
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

extern "C" {
  #include <gpod/itdb.h>
}

#include "Meta.h"

#include "kjob.h"

#include <QObject>

class QString;
class QFile;
class QDateTime;
class QMutex;

class IpodCollection;

namespace Ipod
{


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

/* The libgpod backend for all Ipod calls */
    class IpodHandler : public QObject
    {
        Q_OBJECT

        public:
            IpodHandler( IpodCollection *mc, const QString& mountPoint, QObject* parent );
           ~IpodHandler();

	   void detectModel();
	   QString itunesDir( const QString &path = QString() ) const;
	   QString mountPoint() const { return m_mountPoint; }
	   bool openDevice( bool silent=false );
       void copyTrackToDevice( const Meta::TrackPtr &track );
       bool kioCopyTrack( const KUrl &src, const KUrl &dst );

       void insertTrackIntoDB( const KUrl &url, const Meta::TrackPtr &track );
       void updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track );
       QString           ipodPath( const QString &realPath );
       KUrl determineURLOnDevice( const Meta::TrackPtr &track );
	   void parseTracks();
	   void setMountPoint( const QString &mp) { m_mountPoint = mp; }
       QString           realPath( const char *ipodPath );
	   bool pathExists( const QString &ipodPath, QString *realPath=0 );
	   bool writeITunesDB( bool threaded=true );
           
        public slots:
	    bool initializeIpod();
	void fileTransferred( KJob *job );

        signals:

        private:
            IpodCollection *m_memColl;
	    
        // ipod database
        Itdb_iTunesDB    *m_itdb;
        Itdb_Playlist    *m_masterPlaylist;
//        Q3Dict<Itdb_Track> m_files;

        // podcasts
        Itdb_Playlist*    m_podcastPlaylist;

        bool              m_isShuffle;
        bool              m_isMobile;
	bool              m_isIPhone;

        bool              m_supportsArtwork;
        bool              m_supportsVideo;
	bool              m_rockboxFirmware;
        bool              m_needsFirewireGuid;
        bool              m_autoConnect;

	QString           m_mountPoint;
	QString           m_name;

//        IpodMediaItem    *getArtist( const QString &artist );
//        IpodMediaItem    *getAlbum( const QString &artist, const QString &album );
//        IpodMediaItem    *getTrack( const QString &artist, const QString &album,
//                                    const QString &title,
//                                    int discNumber = -1, int trackNumber = -1 );
//        IpodMediaItem    *getTrack( const Itdb_Track *itrack );

//        bool              removeDBTrack( Itdb_Track *track );

        bool              m_dbChanged;

//        QCheckBox        *m_syncStatsCheck;
//        QCheckBox        *m_autoDeletePodcastsCheck;
        QFile            *m_lockFile;
//        QMutex            m_mutex;

//        KAction          *m_customAction;
//        enum              { CHECK_INTEGRITY, UPDATE_ARTWORK, SET_IPOD_MODEL };

        // KIO-related Vars (to be moved elsewhere eventually)

        bool m_copyFailed;
        bool m_isCanceled;
        bool m_wait;



    };
}
#endif
