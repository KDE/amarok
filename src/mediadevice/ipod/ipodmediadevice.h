/***************************************************************************
    copyright            : (C) 2005, 2006 by Martin Aumueller
    email                : aumuell@reserv.at

    copyright            : (C) 2004 by Christian Muehlhaeuser
    email                : chris@chris.de
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU General Public License version 2 as    *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef AMAROK_IPODMEDIADEVICE_H
#define AMAROK_IPODMEDIADEVICE_H

extern "C" {
#include <gpod/itdb.h>
}


#include "mediabrowser.h"

#include <qmutex.h>
#include <qptrlist.h>
#include <qdict.h>

#include <kio/job.h>

class QCheckBox;
class QLabel;
class QLineEdit;
class QFile;

class KAction;

class IpodMediaItem;
class PodcastInfo;

class IpodMediaDevice : public MediaDevice
{
    friend class IpodMediaItem;
    Q_OBJECT

    public:
        IpodMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~IpodMediaDevice();
        virtual bool      autoConnect()          { return m_autoConnect; }
        virtual bool      asynchronousTransfer() { return false; /* kernel buffer flushes freeze Amarok */ }
        QStringList       supportedFiletypes();

        bool              isConnected();

        virtual void      addConfigElements( QWidget *parent );
        virtual void      removeConfigElements( QWidget *parent );
        virtual void      applyConfig();
        virtual void      loadConfig();
        virtual MediaItem*tagsChanged( MediaItem *item, const MetaBundle &bundle );

        virtual KAction  *customAction() { return m_customAction; }

    protected:
        MediaItem        *trackExists( const MetaBundle& bundle );

        bool              openDevice( bool silent=false );
        bool              closeDevice();
        bool              lockDevice(bool tryLock=false ) { if( tryLock ) { return m_mutex.tryLock(); } else { m_mutex.lock(); return true; } }
        void              unlockDevice() { m_mutex.unlock(); }
        void              initView();
        void              detectModel();

        virtual MediaItem *copyTrackToDevice( const MetaBundle& bundle );
        /**
         * Insert track already located on media device into the device's database
         * @param pathname Location of file on the device to add to the database
         * @param bundle MetaBundle of track
         * @param podcastInfo PodcastInfo of track if it is a podcast, 0 otherwise
         * @return If successful, the created MediaItem in the media device view, else 0
         */
        virtual MediaItem *insertTrackIntoDB( const QString &pathname,
                const MetaBundle &metaBundle, const MetaBundle &propertiesBundle,
                const PodcastInfo *podcastInfo );

        virtual MediaItem *updateTrackInDB( IpodMediaItem *item, const QString &pathname,
                const MetaBundle &metaBundle, const MetaBundle &propertiesBundle,
                const PodcastInfo *podcastInfo );

        /**
         * Determine the url for which a track should be uploaded to on the device
         * @param bundle MetaBundle of track to base pathname creation on
         * @return the url to upload the track to
         */
        virtual KURL determineURLOnDevice( const MetaBundle& bundle );

        void              synchronizeDevice();
        int               deleteItemFromDevice( MediaItem *item, int flags=DeleteTrack );
        virtual void      deleteFile( const KURL &url );
        void              addToPlaylist( MediaItem *list, MediaItem *after, QPtrList<MediaItem> items );
        MediaItem        *newPlaylist( const QString &name, MediaItem *list, QPtrList<MediaItem> items );
        bool              getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
        void              rmbPressed( QListViewItem* qitem, const QPoint& point, int );
        bool              checkIntegrity();
        void              updateArtwork();

    protected slots:
        void              renameItem( QListViewItem *item );
        virtual void      fileDeleted( KIO::Job *job );

    private:
        bool              initializeIpod();
        bool              writeITunesDB( bool threaded=true );
        bool              createLockFile( bool silent );
        IpodMediaItem    *addTrackToView( Itdb_Track *track, IpodMediaItem *item = 0,
                                          bool checkIntegrity = false, bool batchmode = false );
        void              addPlaylistToView( Itdb_Playlist *playlist );
        void              playlistFromItem( IpodMediaItem *item );

        QString           itunesDir( const QString &path = QString::null ) const;
        QString           realPath( const char *ipodPath );
        QString           ipodPath( const QString &realPath );
        bool              pathExists( const QString &ipodPath, QString *realPath=0 );

        // ipod database
        Itdb_iTunesDB    *m_itdb;
        Itdb_Playlist    *m_masterPlaylist;
        QDict<Itdb_Track> m_files;

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

        IpodMediaItem    *getArtist( const QString &artist );
        IpodMediaItem    *getAlbum( const QString &artist, const QString &album );
        IpodMediaItem    *getTrack( const QString &artist, const QString &album,
                                    const QString &title,
                                    int discNumber = -1, int trackNumber = -1,
                                    const PodcastEpisodeBundle *peb = 0 );
        IpodMediaItem    *getTrack( const Itdb_Track *itrack );

        bool              removeDBTrack( Itdb_Track *track );

        bool              m_dbChanged;

        QCheckBox        *m_syncStatsCheck;
        QCheckBox        *m_autoDeletePodcastsCheck;
        QFile            *m_lockFile;
        QMutex            m_mutex;

        KAction          *m_customAction;
        enum              { CHECK_INTEGRITY, UPDATE_ARTWORK, SET_IPOD_MODEL };

    private slots:
        void              slotIpodAction( int );
};

#endif
