// (c) 2004 Christian Muehlhaeuser <chris@chris.de>, 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef AMAROK_GPODMEDIADEVICE_H
#define AMAROK_GPODMEDIADEVICE_H

#include "config.h"

#include "mediabrowser.h"

#ifdef HAVE_LIBGPOD
extern "C" {
#include <gpod/itdb.h>
};
#endif

#include <qptrlist.h>
#include <qdict.h>

class GpodMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
        GpodMediaDevice( MediaDeviceView* parent );
        virtual ~GpodMediaDevice();

        bool        isConnected();
        QStringList items( QListViewItem* item );

    protected:
        bool             trackExists( const MetaBundle& bundle );

        bool             openDevice(bool useDialogs=true);
        bool             closeDevice();
        void lockDevice(bool) {}
        void unlockDevice() {}

        void synchronizeDevice();
        QString createPathname(const MetaBundle& bundle);
        bool addTrackToDevice(const QString& pathname, const MetaBundle& bundle, bool isPodcast);
        bool deleteTrackFromDevice(const QString& artist, const QString& album, const QString& track);

    private:
#ifdef HAVE_LIBGPOD
        void             writeITunesDB();
        void             addTrackToList(Itdb_Track *track);
        void             addPlaylistToList(Itdb_Playlist *playlist);

        QString          realPath(const char *ipodPath);
        QString          ipodPath(const char *realPath);

        Itdb_iTunesDB*   m_itdb;

        struct IpodAlbum : public QDict<Itdb_Track>
        {
           typedef QDictIterator<Itdb_Track> Iterator;
        };
        struct IpodArtist : public QDict<IpodAlbum>
        {
              IpodArtist() { setAutoDelete(true); }
              typedef QDictIterator<IpodAlbum> Iterator;
        };
        struct IpodDB : public QDict<IpodArtist>
        {
              IpodDB() { setAutoDelete(true); }
              typedef QDictIterator<IpodDB> Iterator;
        };

        IpodDB m_database;

        struct IpodPlaylist : public QPtrList<Itdb_Track>
        {
            IpodPlaylist() { m_dbPlaylist=NULL; }
            Itdb_Playlist *m_dbPlaylist;
            typedef QPtrListIterator<Itdb_Track> Iterator;
        };

        QDict<IpodPlaylist> m_playlists;

        IpodArtist *getArtist(const QString &artist);
        IpodAlbum *getAlbum(const QString &artist, const QString &album);
        Itdb_Track *getTitle(const QString &artist, const QString &album, const QString &title);

        bool removeDBTrack(Itdb_Track *track);

        bool dbChanged;

#endif
};

#endif
