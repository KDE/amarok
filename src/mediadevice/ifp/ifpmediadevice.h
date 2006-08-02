/***************************************************************************
 * copyright            : (C) 2005-2006 Seb Ruiz <me@sebruiz.net>          *
 *                                                                         *
 * With some code helpers from KIO_IFP                                     *
 *                        (c) 2004 Thomas Loeber <ifp@loeber1.de>          *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_IFPMEDIADEVICE_H
#define AMAROK_IFPMEDIADEVICE_H

extern "C" {
    #include <ifp.h>
    #include <usb.h>
}

#include "mediabrowser.h"

#include <kurl.h>

#include <qptrlist.h>

class IfpMediaItem;
class TransferDialog;

class IfpMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
                          IfpMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~IfpMediaDevice();

        bool              isConnected() { return m_connected; }
        void              rmbPressed( QListViewItem* qitem, const QPoint& point, int );
        bool              hasTransferDialog() { return true; }
        void              runTransferDialog();
        TransferDialog   *getTransferDialog() { return m_td; }

    protected:
        bool              openDevice( bool silent=false );
        bool              closeDevice();

        bool              lockDevice( bool ) { return true; }
        void              unlockDevice() {}
        void              synchronizeDevice() {}

        MediaItem        *copyTrackToDevice( const MetaBundle& bundle);
        int               deleteItemFromDevice( MediaItem *item, int flags=DeleteTrack );
        bool              getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
        MediaItem        *newDirectory( const QString &name, MediaItem *parent );
        void              addToDirectory( MediaItem *directory, QPtrList<MediaItem> items );

    protected slots:
        void              renameItem( QListViewItem *item );
        void              expandItem( QListViewItem *item );

    private:
        enum              Error { ERR_ACCESS_DENIED, ERR_CANNOT_RENAME, ERR_DISK_FULL, ERR_COULD_NOT_WRITE };

        // Too expensive to implement on a non-database device
        MediaItem        *trackExists( const MetaBundle& ) { return 0; }

        bool              checkResult( int result, QString message );

        // file transfer
        MediaItem        *newDirectoryRecursive( const QString &name, MediaItem *parent );
        int               uploadTrack( const QCString& src, const QCString& dest );
        void              downloadSelectedItems();
        int               downloadTrack( const QCString& src, const QCString& dest );

        // listDir
        void              listDir( const QString &dir );
        static int        listDirCallback( void *pData, int type, const char *name, int size );
        int               addTrackToList( int type, QString name, int size=0 );

        // miscellaneous methods
        static int        filetransferCallback( void *pData, struct ifp_transfer_status *progress );
        int               setProgressInfo( struct ifp_transfer_status *progress );
        // Will iterate over parents and add directory name to the item.
        // getFilename = false will return only parent structure, as opposed to returning the filename as well
        QString           getFullPath( const QListViewItem *item, const bool getFilename = true );
        QString           cleanPath( const QString &component );
        
        MediaItem        *findChildItem( const QString &name, MediaItem *parent );

        // IFP device
        struct usb_device *m_dev;
        usb_dev_handle    *m_dh;
        struct ifp_device  m_ifpdev;

        bool               m_connected;

        IfpMediaItem      *m_last;
        //used to specify new IfpMediaItem parent. Make sure it is restored to 0 (m_listview)
        QListViewItem     *m_tmpParent;
        TransferDialog    *m_td;
};

#endif /*AMAROK_IFPMEDIADEVICE_H*/

