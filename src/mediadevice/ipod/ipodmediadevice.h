// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef AMAROK_IPODMEDIADEVICE_H
#define AMAROK_IPODMEDIADEVICE_H

extern "C" {
#include <gpod/itdb.h>
}

#include "kiomediadevice.h"

#include <qptrlist.h>
#include <qdict.h>

#include <kio/job.h>

class QCheckBox;
class QLabel;
class QLineEdit;
class QFile;

class IpodMediaItem;
class PodcastInfo;

class IpodMediaDevice : public KioMediaDevice
{
    friend class IpodMediaItem;
    Q_OBJECT

    public:
        IpodMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~IpodMediaDevice();
        virtual bool      autoConnect() { return true; }
        virtual bool      asynchronousTransfer() { return false; /* kernel buffer flushes freeze amaroK */ }
        QStringList       supportedFiletypes();

        bool              isConnected();

        virtual void      addConfigElements( QWidget *parent );
        virtual void      removeConfigElements( QWidget *parent );
        virtual void      applyConfig();
        virtual void      loadConfig();
        virtual MediaItem*tagsChanged( MediaItem *item, const MetaBundle &bundle );

    protected:
        MediaItem        *trackExists( const MetaBundle& bundle );

        bool              openDevice( bool silent=false );
        bool              closeDevice();

        virtual MediaItem*copyTrackToDevice(const MetaBundle& bundle);
        /**
         * Insert track already located on media device into the device's database
         * @param pathname Location of file on the device to add to the database
         * @param bundle MetaBundle of track
         * @param podcastInfo PodcastInfo of track if it is a podcast, 0 otherwise
         * @return If successful, the created MediaItem in the media device view, else 0
         */
        virtual MediaItem *insertTrackIntoDB( const QString& pathname, const MetaBundle& bundle, const PodcastInfo *podcastInfo);

        virtual MediaItem * updateTrackInDB(IpodMediaItem *item,
                            const QString &pathname, const MetaBundle &bundle, const PodcastInfo *podcastInfo);


        /**
         * Determine the url for which a track should be uploaded to on the device
         * @param bundle MetaBundle of track to base pathname creation on
         * @return the url to upload the track to
         */
        virtual KURL determineURLOnDevice(const MetaBundle& bundle);

        void              synchronizeDevice();
        int               deleteItemFromDevice(MediaItem *item, bool onlyPlayed=false );
        void              addToPlaylist(MediaItem *list, MediaItem *after, QPtrList<MediaItem> items);
        MediaItem        *newPlaylist(const QString &name, MediaItem *list, QPtrList<MediaItem> items);
        bool              getCapacity(KIO::filesize_t *total, KIO::filesize_t *available);
        void              rmbPressed( QListViewItem* qitem, const QPoint& point, int );

    protected slots:
        void              renameItem( QListViewItem *item );

    private:
        bool              initializeIpod( const QString &mountpoint );
        bool              writeITunesDB( bool threaded=true );
        bool              createLockFile( const QString &mountpoint, bool silent );
        IpodMediaItem    *addTrackToView(Itdb_Track *track, IpodMediaItem *item=0);
        void              addPlaylistToView(Itdb_Playlist *playlist);
        void              playlistFromItem(IpodMediaItem *item);

        QString           realPath(const char *ipodPath);
        QString           ipodPath(const QString &realPath);

        // ipod database
        Itdb_iTunesDB    *m_itdb;
        Itdb_Playlist    *m_masterPlaylist;
        QDict<Itdb_Track> m_files;

        // podcasts
        Itdb_Playlist*    m_podcastPlaylist;

        bool              m_isShuffle;
        bool              m_supportsArtwork;
        bool              m_supportsVideo;

        IpodMediaItem    *getArtist(const QString &artist);
        IpodMediaItem    *getAlbum(const QString &artist, const QString &album);
        IpodMediaItem    *getTrack(const QString &artist, const QString &album, const QString &title, int trackNumber=-1);
        IpodMediaItem    *getTrack(const Itdb_Track *itrack);

        bool              removeDBTrack(Itdb_Track *track);

        bool              m_dbChanged;

        QCheckBox        *m_syncStatsCheck;
        QCheckBox        *m_autoDeletePodcastsCheck;
        QFile            *m_lockFile;
};

#endif
