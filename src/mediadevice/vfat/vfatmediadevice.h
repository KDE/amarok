/***************************************************************************
 * copyright            : (C) 2006 Jeff Mitchell <kde-dev@emailgoeshere.com *
 *                        (C) 2005 Seb Ruiz <me@sebruiz.net>                *
 *                                                                          *
 * With some code helpers from KIO_VFAT                                     *
 *                        (c) 2004 Thomas Loeber <vfat@loeber1.de>          *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_VFATMEDIADEVICE_H
#define AMAROK_VFATMEDIADEVICE_H

#include "../mediabrowser.h"
#include "transferdialog.h"

#include <kdirlister.h>
#include <kurl.h>

// #include <qbitarray.h>
#include <qptrlist.h>

class VfatMediaItem;

class VfatMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
                          VfatMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~VfatMediaDevice();

        bool              isConnected() { return m_connected; }
        void              rmbPressed( QListViewItem* qitem, const QPoint& point, int );
        bool              hasTransferDialog() { return true; }
        void              runTransferDialog();
        TransferDialog   *getTransferDialog() { return m_td; }
        bool              needsManualConfig() { return false; }

    protected:
        bool              openDevice( bool silent=false );
        bool              closeDevice();

        bool              lockDevice( bool ) { return true; }
        void              unlockDevice() {}
        void              synchronizeDevice() {}

        MediaItem        *copyTrackToDevice( const MetaBundle& bundle, const PodcastInfo *info );
        int               deleteItemFromDevice( MediaItem *item, bool onlyPlayed = false );
        bool              getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
        MediaItem        *newDirectory( const QString &name, MediaItem *parent );
        void              addToDirectory( MediaItem *directory, QPtrList<MediaItem> items );

        void              addToPlaylist( MediaItem *, MediaItem *, QPtrList<MediaItem> ) {}
        MediaItem        *newPlaylist( const QString &, MediaItem *, QPtrList<MediaItem> ) { return 0; }

        void              cancelTransfer() {} // we don't have to do anything, we check m_cancelled

    signals:
        void              startTransfer();

    protected slots:
        void              renameItem( QListViewItem *item );
        void              expandItem( QListViewItem *item );
        void              foundMountPoint( const QString & mountPoint, unsigned long kBSize, unsigned long kBUsed, unsigned long kBAvail );
        void              newItems( const KFileItemList &items );
        void              dirListerCompleted();

    private:
        enum              Error { ERR_ACCESS_DENIED, ERR_CANNOT_RENAME, ERR_DISK_FULL, ERR_COULD_NOT_WRITE };

        MediaItem        *trackExists( const MetaBundle& );

        bool              checkResult( int result, QString message );

        // file transfer
        void              downloadSelectedItems();
        void              copyTrackSortHelper( const MetaBundle& bundle, QString& sort, QString& temp, QString& base );

        // listDir
        void              listDir( const QString &dir );
        static int        listDirCallback( void *pData, int type, const char *name, int size );
        int               addTrackToList( int type, QString name, int size=0 );

        // miscellaneous methods
        //static int        filetransferCallback( void *pData, struct vfat_transfer_status *progress );
        //int               setProgressInfo( struct vfat_transfer_status *progress );
        // Will iterate over parents and add directory name to the item.
        // getFilename = false will return only parent structure, as opposed to returning the filename as well
        QString           getFullPath( const QListViewItem *item, const bool getFilename = true, const bool prependMount = true, const bool clean = true );

        QString           cleanPath( const QString &component );

        bool              m_connected;

        VfatMediaItem     *m_last;
        //used to specify new VfatMediaItem parent. Make sure it is restored to 0 (m_listview)
        QListViewItem     *m_tmpParent;

        unsigned long     m_kBSize;
        unsigned long     m_kBAvail;

        KDirLister        *m_dirLister;
        KIO::UDSEntry     m_udsentry;

        TransferDialog    *m_td;
        bool              m_actuallyVfat;
        bool              m_isInCopyTrack;
        bool              m_stopDirLister;

};

#endif /*AMAROK_VFATMEDIADEVICE_H*/

