/****************************************************************************
 * copyright            :(C) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com> *
 *                       (C) 2005 Seb Ruiz <me@sebruiz.net>                 *
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

#include <qptrlist.h>

class VfatMediaItem;
class VfatMediaFile;

typedef QMap<QString, VfatMediaFile*> MediaFileMap;
typedef QMap<VfatMediaItem*, VfatMediaFile*> MediaItemMap;

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
        void              loadConfig();

        MediaFileMap     &getFileMap() { return m_mfm; }
        MediaItemMap     &getItemMap() { return m_mim; }

    protected:
        bool              openDevice( bool silent=false );
        bool              closeDevice();

        bool              lockDevice( bool ) { return true; }
        void              unlockDevice() {}
        void              synchronizeDevice() {}

        //TODO
        MediaItem        *copyTrackToDevice( const MetaBundle& bundle );
        //TODO
        int               deleteItemFromDevice( MediaItem *item, bool onlyPlayed = false );
        bool              getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
        MediaItem        *newDirectory( const QString &name, MediaItem *parent );
        void              addToDirectory( MediaItem *directory, QPtrList<MediaItem> items );

        void              addToPlaylist( MediaItem *, MediaItem *, QPtrList<MediaItem> ) {}
        MediaItem        *newPlaylist( const QString &, MediaItem *, QPtrList<MediaItem> ) { return 0; }
        QString           fileName( const MetaBundle & );

    signals:
        void              startTransfer();

    protected slots:
        void              renameItem( QListViewItem *item );
        void              expandItem( QListViewItem *item );
        void              foundMountPoint( const QString & mountPoint, unsigned long kBSize, unsigned long kBUsed, unsigned long kBAvail );
        void              refreshDir( const QString &dir );

        void              newItems( const KFileItemList &items );
        void              dirListerCompleted();
        void              dirListerClear();
        void              dirListerClear( const KURL &url );
        void              dirListerDeleteItem( KFileItem *fileitem );

        //TODO
        void              downloadSlotRedirection( KIO::Job *job, const KURL &url );
        //TODO
        void              downloadSlotResult( KIO::Job *job );
        //TODO
        void              downloadSlotEntries( KIO::Job *, const KIO::UDSEntryList &entries );

    private:
        enum              Error { ERR_ACCESS_DENIED, ERR_CANNOT_RENAME, ERR_DISK_FULL, ERR_COULD_NOT_WRITE };

        MediaItem        *trackExists( const MetaBundle& );

        // file transfer
        //TODO
        void              downloadSelectedItems();

        //TODO
        void              copyTrackSortHelper( const MetaBundle& bundle, QString& sort, QString& temp, QString& base );

        KURL::List        getSelectedItems();

        // listDir
        void              listDir( const QString &dir );
        int               addTrackToList( int type, KURL name, int size=0 );

        // miscellaneous methods

        //TODO: remove when possible!
        QString           getFullPath( const QListViewItem *item, const bool getFilename = true, const bool prependMount = true, const bool clean = true );

        QString           cleanPath( const QString &component );

        VfatMediaItem     *m_last;
        VfatMediaFile     *m_initialFile;
        //TODO: remove when possible!
        //used to specify new VfatMediaItem parent. Make sure it is restored to 0 (m_listview)
        QListViewItem     *m_tmpParent;

        KIO::filesize_t   m_kBSize;
        KIO::filesize_t   m_kBAvail;

        KDirLister        *m_dirLister;
        KIO::UDSEntry     m_udsentry;

        TransferDialog    *m_td;
        bool              m_actuallyVfat;
        bool              m_isInCopyTrack;
        bool              m_stopDirLister;
        bool              m_dirListerComplete;
        bool              m_connected;
        KURL::List        m_downloadList;
        bool              m_downloadListerFinished;
        KURL              m_currentJobUrl;
        MediaFileMap      m_mfm;
        MediaItemMap      m_mim;
};

#endif /*AMAROK_VFATMEDIADEVICE_H*/

