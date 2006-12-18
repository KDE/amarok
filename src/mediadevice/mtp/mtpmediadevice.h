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
  *  Based on njb mediadevice with some code hints from the libmtp
  *  example tools
  */

 /**
  *  MTP media device
  *  @author Andy Kelk <andy@mopoke.co.uk>
  *  @see http://libmtp.sourceforge.net/
  */

#ifndef AMAROK_MTPMEDIADEVICE_H
#define AMAROK_MTPMEDIADEVICE_H

#include "mediabrowser.h"

#include <qptrlist.h>
#include <qmutex.h>

#include <libmtp.h>

class MtpMediaDevice;
class MtpMediaItem;

class MtpTrack {
    friend class MediaItem;
    public:
        MtpTrack( LIBMTP_track_t* track );
        ~MtpTrack() {};
        bool                    operator==( const MtpTrack& second ) const { return m_id == second.m_id; }

    public:
        u_int32_t               id() const { return m_id; }
        MetaBundle              *bundle() { return new MetaBundle( m_bundle ); }
        uint32_t                folderId() const { return m_folder_id; }
        void                    setBundle( MetaBundle &bundle );
        void                    setId( int id ) { m_id = id; }
        void                    setFolderId( const uint32_t folder_id ) { m_folder_id = folder_id; }
        void                    readMetaData( LIBMTP_track_t *track );

    private:
        u_int32_t               m_id;
        MetaBundle              m_bundle;
        uint32_t                m_folder_id;
};

class MtpPlaylist {
    friend class MediaItem;
    public:
        bool                    operator==( const MtpPlaylist& second ) const { return m_id == second.m_id; }

    public:
        u_int32_t               id() const { return m_id; }
        void                    setId( int id ) { m_id = id; }

    private:
        u_int32_t               m_id;
};


class MtpAlbum {
    friend class MediaItem;
    public:
        MtpAlbum( LIBMTP_album_t* album );
        ~MtpAlbum();
        bool                    operator==( const MtpAlbum& second ) const { return m_id == second.m_id; }

    public:
        u_int32_t               id() const { return m_id; }
        void                    setId( int id ) { m_id = id; }
        QString                 album() const { return m_album; }

    private:
        u_int32_t               m_id;
        QString                 m_album;
};

class MtpMediaItem : public MediaItem
{
    public:
        MtpMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after ) {}
        MtpMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after ) {}
        MtpMediaItem( QListView *parent, MediaDevice *dev )
            : MediaItem( parent ) { init( dev ); }
        MtpMediaItem( QListViewItem *parent, MediaDevice *dev )
            : MediaItem( parent ) { init( dev ); }

        void init( MediaDevice *dev )
        {
            m_track  = 0;
            m_playlist = 0;
            m_device = dev;
        }

        ~MtpMediaItem()
        {
            //m_track->removeItem(this);
        }
        void                setTrack( MtpTrack *track ) { m_track = track; }
        MtpTrack            *track() { return m_track; }
        void                setPlaylist( MtpPlaylist *playlist ) { m_playlist = playlist; }
        MtpPlaylist         *playlist() { return m_playlist; }
        QString             filename() { return m_track->bundle()->url().path(); }

    private:
        MtpTrack            *m_track;
        MtpPlaylist         *m_playlist;
};

class MtpMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
        MtpMediaDevice();
        virtual bool            autoConnect()          { return false; }
        virtual bool            asynchronousTransfer() { return false; }
        bool                    isConnected();
        LIBMTP_mtpdevice_t      *current_device();
        void                    setDisconnected();
        virtual void            rmbPressed( QListViewItem *qitem, const QPoint &point, int arg1 );
        virtual void            init( MediaBrowser* parent );
        virtual QStringList     supportedFiletypes();
        void                    setFolders( LIBMTP_folder_t *folders );
        void                    cancelTransfer();
        void                    customClicked();
        virtual void            addConfigElements( QWidget *parent );
        virtual void            removeConfigElements( QWidget *parent );
        virtual void            applyConfig();
        virtual void            loadConfig();
        static int              progressCallback( uint64_t const sent, uint64_t const total, void const * const data );

    protected:
        MediaItem*              trackExists( const MetaBundle &bundle );

        bool                    openDevice( bool silent );
        bool                    closeDevice();
        bool                    lockDevice( bool tryLock=false ) { if( tryLock ) { return m_mutex.tryLock(); } else { m_mutex.lock(); return true; } }
        void                    unlockDevice() { m_mutex.unlock(); }

        MediaItem               *copyTrackToDevice( const MetaBundle &bundle );
        int                     downloadSelectedItemsToCollection();

        void                    synchronizeDevice();
        int                     deleteItemFromDevice( MediaItem *mediaitem, int flags=DeleteTrack );
        void                    addToPlaylist( MediaItem *list, MediaItem *after, QPtrList<MediaItem> items );
        MtpMediaItem            *newPlaylist( const QString &name, MediaItem *list, QPtrList<MediaItem> items );
        bool                    getCapacity( KIO::filesize_t *total, KIO::filesize_t *available );
        virtual void            updateRootItems() {};

    private slots:
        void                    playlistRenamed( QListViewItem *item, const QString &, int );

    private:
        MtpMediaItem            *addTrackToView(MtpTrack *track, MtpMediaItem *item=0 );
        int                     readMtpMusic( void );
        void                    clearItems();
        int                     deleteObject( MtpMediaItem *deleteItem );
        uint32_t                checkFolderStructure( const MetaBundle &bundle, bool create = true );
        uint32_t                createFolder( const char *name, uint32_t parent_id );
        uint32_t                getDefaultParentId( void );
        uint32_t                folderNameToID( char *name, LIBMTP_folder_t *folderlist );
        uint32_t                subfolderNameToID( const char *name, LIBMTP_folder_t *folderlist, uint32_t parent_id );
        void                    updateFolders( void );
        void                    initView( void );
        void                    readPlaylists( void );
        void                    readAlbums( void );
        void                    playlistFromItem( MtpMediaItem *item);
        QByteArray              *getSupportedImage( QString path );
        void                    sendAlbumArt( QPtrList<MediaItem> *items );
        void                    updateAlbumArt( QPtrList<MediaItem> *items );
        LIBMTP_album_t          *getOrCreateAlbum( QPtrList<MediaItem> *items );
        LIBMTP_mtpdevice_t      *m_device;
        QMutex                  m_mutex;
        QMutex                  m_critical_mutex;
        LIBMTP_folder_t         *m_folders;
        uint32_t		        m_default_parent_folder;
        QString                 m_folderStructure;
        QLineEdit               *m_folderStructureBox;
        QLabel                  *m_folderLabel;
        QStringList             m_supportedFiles;
        QPtrList<MediaItem>     *m_newTracks;
        QMap<int,QString>       mtpFileTypes;
        QMap<uint32_t,MtpTrack*> m_idToTrack;
        QMap<QString,MtpMediaItem*> m_fileNameToItem;
        QMap<uint32_t,MtpAlbum*> m_idToAlbum;
        QString                 m_format;
};

#endif
