// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef AMAROK_KIOMEDIADEVICE_H
#define AMAROK_KIOMEDIADEVICE_H

#include "mediabrowser.h"

#include <qmutex.h>
#include <qptrlist.h>

#include <kio/job.h>

class KioMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
                          KioMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~KioMediaDevice();
        virtual bool      asynchronousTransfer() { return true; }

        bool              isConnected();

    protected:
        virtual MediaItem*trackExists( const MetaBundle& bundle );

        bool              openDevice( bool silent=false );
        bool              closeDevice();
        bool              lockDevice(bool tryLock=false ) { if( tryLock ) { return m_mutex.tryLock(); } else { m_mutex.lock(); return true; } }
        void              unlockDevice() { m_mutex.unlock(); }

        /**
         * Determine the url for which a track should be uploaded to on the device
         * @param bundle MetaBundle of track to base pathname creation on
         * @return the url to upload the track to
         */
        virtual KURL      determineURLOnDevice(const MetaBundle& bundle);

        virtual void      synchronizeDevice();
        virtual int       deleteItemFromDevice(MediaItem *item, bool onlyPlayed=false );
        virtual bool      getCapacity(KIO::filesize_t *total, KIO::filesize_t *available);
        virtual void      rmbPressed( MediaView *deviceList, QListViewItem* qitem, const QPoint& point, int );
        virtual void      deleteFile( const KURL &url );
        virtual void      abortTransfer() {}
        QMutex m_mutex;

    protected slots:
        virtual void      fileDeleted( KIO::Job *job );

    private:
        MediaItem        *getArtist(const QString &artist);
        MediaItem        *getAlbum(const QString &artist, const QString &album);
        MediaItem        *getTrack(const QString &artist, const QString &album, const QString &title, int trackNumber=-1);
};

#endif
