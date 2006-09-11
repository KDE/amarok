/***************************************************************************
 * copyright            : (C) 2006 Andy Kelk <andy@mopoke.co.uk>           *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /**
  *  Rio Karma media device
  *  @author Andy Kelk <andy@mopoke.co.uk>
  *  @see http://linux-karma.sourceforge.net/
  */

#ifndef AMAROK_RIOKARMAMEDIADEVICE_H
#define AMAROK_RIOKARMAMEDIADEVICE_H

#include "mediabrowser.h"

#include <qptrlist.h>
#include <qmutex.h>

#include "libkarma/lkarma.h"

class RioKarmaMediaDevice;
class RioKarmaMediaItem;

class RioKarmaTrack {
    friend class MediaItem;
    public:
        RioKarmaTrack( int Fid );
        ~RioKarmaTrack();
        bool                        operator==( const RioKarmaTrack &second ) const { return m_id == second.m_id; }

    public:
        unsigned int                id() const { return m_id; }
        MetaBundle                  *bundle() { return new MetaBundle( m_bundle ); }
        void                        setBundle( MetaBundle &bundle );
        void                        setId( int id ) { m_id = id; }
        void                        readMetaData();
        void                        addItem( const RioKarmaMediaItem *item );
        bool                        removeItem( const RioKarmaMediaItem *item );
    private:
        unsigned int                m_id;
        MetaBundle                  m_bundle;
        QPtrList<RioKarmaMediaItem> m_itemList;
};


class RioKarmaMediaItem : public MediaItem
{
    public:
        RioKarmaMediaItem( QListView *parent, QListViewItem *after = 0 ) : MediaItem( parent, after )
        {}
        RioKarmaMediaItem( QListViewItem *parent, QListViewItem *after = 0 ) : MediaItem( parent, after )
        {}
        ~RioKarmaMediaItem()
        {
            //m_track->removeItem(this);
        }
        void                        setTrack( RioKarmaTrack *track ) { m_track = track; m_track->addItem( this ); }
        RioKarmaTrack               *track() { return m_track; }
        QString                     filename() { return m_track->bundle()->url().path(); }
    private:
        RioKarmaTrack               *m_track;
};


class trackValueList: public QValueList<RioKarmaTrack *>
{
    public:
        trackValueList::iterator        findTrackById( unsigned );
        trackValueList::const_iterator  findTrackById( unsigned ) const;
        trackValueList::iterator        findTrackByName( const QString& );
        int                             readFromDevice( RioKarmaMediaDevice* rio );
};


class RioKarmaMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
        RioKarmaMediaDevice();
        virtual bool                autoConnect() { return true; }
        virtual bool                asynchronousTransfer() { return false; }
        bool                        isConnected();
        int                         current_id();
        void                        setDisconnected();
        virtual void                rmbPressed( QListViewItem *qitem, const QPoint &point, int arg1 );
        virtual void                init( MediaBrowser *parent );
        virtual QStringList         supportedFiletypes();

    public slots:
        void                        expandItem( QListViewItem *item );



    protected:
        MediaItem*                  trackExists( const MetaBundle &bundle );

        bool                        openDevice( bool silent );
        bool                        closeDevice();
        bool                        lockDevice( bool tryLock=false ) { if( tryLock ) { return m_mutex.tryLock(); } else { m_mutex.lock(); return true; } }
        void                        unlockDevice() { m_mutex.unlock(); }

        virtual MediaItem           *copyTrackToDevice( const MetaBundle &bundle );

        void                        synchronizeDevice();
        int                         deleteItemFromDevice( MediaItem *item, int flags=DeleteTrack );
        void                        addToPlaylist( MediaItem *list, MediaItem *after, QPtrList<MediaItem> items );
        RioKarmaMediaItem           *newPlaylist( const QString &name, MediaItem *list, QPtrList<MediaItem> items );
        bool                        getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
        virtual void                updateRootItems() {};

    private:
        RioKarmaMediaItem           *addAlbums( const QString &artist, RioKarmaMediaItem *item );
        RioKarmaMediaItem           *addTracks( const QString &artist, const QString &track, RioKarmaMediaItem *item );
        RioKarmaMediaItem           *addArtist( RioKarmaTrack *track );
        RioKarmaMediaItem           *addTrackToView( RioKarmaTrack *track, RioKarmaMediaItem *item=0 );
        int                         readKarmaMusic( void );
        void                        clearItems();
        int                         deleteRioTrack( RioKarmaMediaItem *trackItem );
        trackValueList              m_trackList;
        int                         m_rio;
        QMutex                      m_mutex;

};

#endif
