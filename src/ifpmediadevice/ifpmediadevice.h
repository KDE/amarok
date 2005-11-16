/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
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

// #include <qbitarray.h>
#include <qptrlist.h>

class IfpMediaItem;

class IfpMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
                          IfpMediaDevice( MediaDeviceView* parent, MediaDeviceList* listview );
        virtual           ~IfpMediaDevice();

        bool              isConnected() { return m_connected; }

    protected:
        MediaItem        *trackExists( const MetaBundle& bundle ) { return 0; }

        bool              openDevice( bool silent=false );
        bool              closeDevice();

        void              lockDevice( bool ) {}
        void              unlockDevice() {}
        void              synchronizeDevice() {}

        MediaItem        *addTrackToDevice( const QString& pathname, const MetaBundle& bundle, bool isPodcast);
        bool              deleteItemFromDevice( MediaItem *item, bool onlyPlayed = false );
        bool              getCapacity(unsigned long *total, unsigned long *available);

        QString           createPathname( const MetaBundle& bundle) { return QString::null; }
        void              addToPlaylist( MediaItem *list, MediaItem *after, QPtrList<MediaItem> items) {}
        MediaItem        *newPlaylist( const QString &name, MediaItem *list, QPtrList<MediaItem> items) { return 0; }

    protected slots:
        void              renameItem( QListViewItem *item );
        void              expandItem( QListViewItem *item );

    private:
        enum              Error { ERR_ACCESS_DENIED, ERR_CANNOT_RENAME, ERR_DISK_FULL, ERR_COULD_NOT_WRITE };

        bool              checkResult( int result, QString message );
        void              unEscape( QString &s );

        // upload
        int               uploadTrack( const QString& src, const QString& dest );
        int               uploadCallback( void *pData, ifp_transfer_status *progress );

        // listDir
        void              listDir( const QString &dir );
        static int        listDirCallback( void *pData, int type, const char *name, int size );
        int               addTrackToList( int type, QString name, int size=0 );

        // IFP device
        struct usb_device *m_dev;
        usb_dev_handle    *m_dh;
        struct ifp_device  m_ifpdev;

//         QByteArray         m_uploadData;
//         int                m_uploadPos;
        bool               m_connected;

        IfpMediaItem      *m_last;
        //used to specify new IfpMediaItem parent. Make sure it is restored to 0 (m_listview)
        QListViewItem     *m_tmpParent;
};

#endif /*AMAROK_IFPMEDIADEVICE_H*/

