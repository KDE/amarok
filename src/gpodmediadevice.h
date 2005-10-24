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
        KURL::List  songsByArtist( const QString& artist );
        KURL::List  songsByArtistAlbum( const QString& artist, const QString& album );

        bool        renameArtist( const QString& oldArtist, const QString& newArtist );
        bool        renameAlbum( const QString& artist, const QString& oldAlbum, const QString& newAlbum );
        bool        renameTrack( const QString& artist, const QString& album,
                        const QString& oldTrack, const QString& newTrack );

    public slots:
        void deleteFiles( const KURL::List& urls );
        void connectDevice();
        void transferFiles();

    private:
        bool             fileExists( const MetaBundle& bundle );

#ifdef HAVE_LIBGPOD
        void             deleteFromIPod( MediaItem* item );
        void             openIPod(bool useDialogs=true);
        void             closeIPod();
        void             writeITunesDB();
        void             addTrackToList(Itdb_Track *track);
        void             addPlaylistToList(Itdb_Playlist *playlist);

        QString          realPath(const char *ipodPath);

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

        bool deleteArtist(const QString &artistName);
        bool deleteAlbum(const QString &artistName, const QString &albumName);
        bool deleteTrack(const QString &artistName, const QString &albumName, Itdb_Track *track);

        Itdb_Track *newDBTrack(const MetaBundle &bundle);
        bool addDBTrack(Itdb_Track *track, bool isPodcast=false);
        bool removeDBTrack(Itdb_Track *track);

        void lock(bool) {}
        void unlock() {}

        bool dbChanged;

        void syncIPod();
#endif
};

#endif
