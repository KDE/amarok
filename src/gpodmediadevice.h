// (c) 2004 Christian Muehlhaeuser <chris@chris.de>, 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef AMAROK_GPODMEDIADEVICE_H
#define AMAROK_GPODMEDIADEVICE_H

#include "config.h"

#ifdef HAVE_LIBGPOD
extern "C" {
#include <gpod/itdb.h>
}
#endif

#include "mediabrowser.h"

#include <qptrlist.h>
#include <qdict.h>

class GpodMediaItem;

class GpodMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
        GpodMediaDevice( MediaDeviceView* parent, MediaDeviceList* listview );
        virtual ~GpodMediaDevice();

        bool        isConnected();

    protected:
        bool             trackExists( const MetaBundle& bundle );

        bool             openDevice(bool useDialogs=true);
        bool             closeDevice();
        void lockDevice(bool) {}
        void unlockDevice() {}

        void synchronizeDevice();
        QString createPathname(const MetaBundle& bundle);
        MediaItem *addTrackToDevice(const QString& pathname, const MetaBundle& bundle, bool isPodcast);
        bool deleteItemFromDevice(MediaItem *item, bool onlyPlayed=false );
        void        addToPlaylist(MediaItem *list, MediaItem *after, QPtrList<MediaItem> items);
        MediaItem*  newPlaylist(const QString &name, MediaItem *list, QPtrList<MediaItem> items);

    protected slots:
        void renameItem( QListViewItem *item );

    private:
#ifdef HAVE_LIBGPOD
        void             writeITunesDB();
        GpodMediaItem   *addTrackToList(Itdb_Track *track);
        void             addPlaylistToList(Itdb_Playlist *playlist);
        void             playlistFromItem(GpodMediaItem *item);

        QString          realPath(const char *ipodPath);
        QString          ipodPath(const char *realPath);

        // ipod database
        Itdb_iTunesDB*   m_itdb;
        Itdb_Playlist*   m_masterPlaylist;
        QDict<Itdb_Track> m_files;

        // podcasts
        Itdb_Playlist*   m_podcastPlaylist;

        GpodMediaItem *getArtist(const QString &artist);
        GpodMediaItem *getAlbum(const QString &artist, const QString &album);
        GpodMediaItem *getTitle(const QString &artist, const QString &album, const QString &title);

        bool removeDBTrack(Itdb_Track *track);

        bool dbChanged;
#endif
};

#endif
