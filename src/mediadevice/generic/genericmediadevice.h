/****************************************************************************
 * copyright            :(C) 2006 Roel Meeuws <r.j.meeuws+amarok@gmail.com> *
 *                       (C) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com> *
 *                       (C) 2005 Seb Ruiz <me@sebruiz.net>                 *
 *                                                                          *
 * With some code helpers from KIO_GENERIC                                  *
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


#ifndef AMAROK_GENERICMEDIADEVICE_H
#define AMAROK_GENERICMEDIADEVICE_H

#include "mediabrowser.h"
#include "transferdialog.h"

#include <kdirlister.h>
#include <kurl.h>

#include <qptrlist.h>

class GenericMediaItem;
class GenericMediaFile;
class GenericMediaDeviceConfigDialog;
class PodcastEpisodeBundle;

class QStringList;

typedef QMap<QString, GenericMediaFile*> MediaFileMap;
typedef QMap<GenericMediaItem*, GenericMediaFile*> MediaItemMap;

class GenericMediaDevice : public MediaDevice
{
    Q_OBJECT

    friend  class GenericMediaDeviceConfigDialog;

    public:
                          GenericMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~GenericMediaDevice();

        bool              isConnected() { return m_connected; }

        void              rmbPressed( QListViewItem* qitem, const QPoint& point, int );

        QStringList       supportedFiletypes() { return m_supportedFileTypes; }
        bool              isPlayable( const MetaBundle& bundle );
        bool              isPreferredFormat( const MetaBundle &bundle );

        bool              needsManualConfig() { return false; }
        void              addConfigElements( QWidget * parent );
        void              removeConfigElements( QWidget * /* parent */);

        void              applyConfig();
        void              loadConfig();

        MediaFileMap     &getFileMap() { return m_mfm; }
        MediaItemMap     &getItemMap() { return m_mim; }
        GenericMediaFile *getInitialFile() { return m_initialFile; }

    protected:
        bool              openDevice( bool silent=false );
        bool              closeDevice();

        MediaItem        *copyTrackToDevice( const MetaBundle& bundle );
        int               deleteItemFromDevice( MediaItem *item, int flags=DeleteTrack );
        MediaItem        *newDirectory( const QString &name, MediaItem *parent );
        void              addToDirectory( MediaItem *directory, QPtrList<MediaItem> items );

        bool              getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
        QString           fileName( const MetaBundle & );

        //methods not implemented/needed
        bool              lockDevice( bool ) { return true; }
        void              unlockDevice() {}
        void              synchronizeDevice() {}
        void              addToPlaylist( MediaItem *, MediaItem *, QPtrList<MediaItem> ) {}
        MediaItem        *newPlaylist( const QString &, MediaItem *, QPtrList<MediaItem> ) { return 0; }


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


    private:
        enum              Error { ERR_ACCESS_DENIED, ERR_CANNOT_RENAME, ERR_DISK_FULL, ERR_COULD_NOT_WRITE };

        MediaItem        *trackExists( const MetaBundle& );

        QString           buildDestination( const QString &format, const MetaBundle &mb );
        QString           buildPodcastDestination( const PodcastEpisodeBundle *bundle );
        void              checkAndBuildLocation( const QString& location );

        KURL::List        getSelectedItems();
        void              downloadSelectedItems();
        void              copyTrackSortHelper( const MetaBundle& bundle, QString& sort, QString& base );

        void              listDir( const QString &dir );
        int               addTrackToList( int type, KURL name, int size=0 );

        QString           cleanPath( const QString &component );

        GenericMediaFile     *m_initialFile;

        KIO::filesize_t   m_kBSize;
        KIO::filesize_t   m_kBAvail;

        KDirLister        *m_dirLister;

        bool              m_actuallyVfat;
        bool              m_dirListerComplete;
        bool              m_connected;
        KURL::List        m_downloadList;
        MediaFileMap      m_mfm;
        MediaItemMap      m_mim;

        QStringList       m_supportedFileTypes;
        QString           m_songLocation;
        QString           m_podcastLocation;
        bool              m_asciiTextOnly;
        bool              m_vfatTextOnly;
        bool              m_ignoreThePrefix;

        GenericMediaDeviceConfigDialog *m_configDialog;
};

#endif /*AMAROK_GENERICMEDIADEVICE_H*/
