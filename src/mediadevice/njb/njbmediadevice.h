//
// C++ Interface: njbmediadevice
//
// Description: This class is used to manipulate Nomad Creative Jukebox and others media player that works with the njb libraries.
//
//
// Author: Andres Oton <andres.oton@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef NJBMEDIADEVICE_H
#define NJBMEDIADEVICE_H

// #include <mediadevice.h>
#include <mediabrowser.h>

#include <libnjb.h>

#include <qptrlist.h>
#include <qlistview.h>

#include "playlist.h"
#include "track.h"

#include <qcstring.h>
#include <qstring.h>
#include <qvaluelist.h>

// kde
#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

/**
  This class is used to manipulate Nomad Creative Jukebox and others media player that works with the njb libraries.

  You can find the njb libs at : http://libnjb.sourceforge.net

  Based at kionjb from 

  @author Andres Oton <andres.oton@gmail.com>
 */

const int NJB_SUCCESS = 0;
const int NJB_FAILURE = -1;

// Global which NJB to use
extern njb_t* theNjb;

// Global track list
extern trackValueList* theTracks;

struct DataFile
{
    QString   filename;
    QString   folder;
    u_int32_t dfid;
    u_int64_t filesize;
};
typedef QValueList<DataFile> dataFileValueList;

class NjbMediaItem;

class NjbMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
        NjbMediaDevice();

        ~NjbMediaDevice();

        //	bool configBool(const QString& name, bool defValue);
        //	bool getSpacesToUnderscores();
        //	bool isCancelled();
        //	bool isDeleting();
        //	bool isTransferring();
        //	int progress() const;

        // 	MediaItem* transferredItem();
        // 	MediaView* view();
        // 	Medium* getMedium();
        // 	QString configString(const QString& name, const QString& defValue);


        // 	QString deviceNode() const;
        // 	QString deviceType();
        // 	QString getTransferDir();
        // 	QString name() const;
        // 	QString uniqueId() const;
        // 	virtual bool asynchronousTransfer();
        // 	virtual bool autoConnect();
        // 	virtual bool hasTransferDialog();

        //Implemented
        virtual bool isConnected();

        //Implemented
        virtual bool isPlayable(const MetaBundle& bundle);

        //Implemented
        virtual bool isPreferredFormat(const MetaBundle& bundle);

        //	virtual bool needsManualConfig();

        //Implementing
        virtual MediaItem* newPlaylist(const QString& name, MediaItem* parent, QPtrList< MediaItem > items);

        //	virtual MediaItem* tagsChanged(MediaItem* item, const MetaBundle& changed);

        virtual QStringList supportedFiletypes();
        virtual TransferDialog* getTransferDialog();
        virtual void addConfigElements(QWidget* arg1);
        virtual void addToDirectory(MediaItem* directory, QPtrList< MediaItem > items);
        virtual void addToPlaylist(MediaItem* playlist, MediaItem* after, QPtrList< MediaItem > items);
        virtual void applyConfig();
        virtual void init(MediaBrowser* parent);
        virtual void loadConfig();
        virtual void removeConfigElements(QWidget* arg1);
        virtual void rmbPressed(QListViewItem* qitem, const QPoint& point, int arg1);
        virtual void runTransferDialog();
        void hideProgress();
        void setConfigBool(const QString& name, bool value);
        void setConfigString(const QString& name, const QString& value);
        void setDeviceType(const QString& type);
        void setFirstSort(QString text);
        void setSecondSort(QString text);
        void setSpacesToUnderscores(bool yesno);
        void setThirdSort(QString text);

    protected:

        // Implemented
        virtual bool closeDevice();

        //Implemented
        virtual bool getCapacity(KIO::filesize_t* total, KIO::filesize_t* available);

        // virtual bool isSpecialItem(MediaItem* item);

        //Implemented
        virtual bool lockDevice(bool tryOnly);

        //Implemented
        virtual bool openDevice(bool silent);

        //Implemented
        int deleteFromDevice(unsigned id);
        virtual int deleteItemFromDevice(MediaItem* item, bool onlyPlayed);
        int deleteAlbum(NjbMediaItem *albumItem);
        int deleteArtist(NjbMediaItem *artistItem);
        int deleteTrack(NjbMediaItem *trackItem);

        int downloadSelectedItems( NjbMediaItem *item );
        int downloadArtist(NjbMediaItem *artistItem);
        int downloadAlbum(NjbMediaItem *albumItem);
        int downloadTrack(NjbMediaItem *trackItem);

        int downloadNow();
        int downloadTrackNow(NjbMediaItem *item, QString path);

        //Implemented
        virtual MediaItem* copyTrackToDevice(const MetaBundle& bundle);

        virtual void cancelTransfer();
        virtual void synchronizeDevice();

        //Implemented
        virtual void unlockDevice();

        virtual void updateRootItems();
        void purgeEmptyItems(MediaItem* root);
        void syncStatsFromDevice(MediaItem* root);
        void syncStatsToDevice(MediaItem* root);

    private:
        // TODO: 
        MediaItem        *trackExists( const MetaBundle& ) { return 0; }

        // miscellaneous methods
        // static int        filetransferCallback( void *pData, struct ifp_transfer_status *progress );
        static int progressCallback( u_int64_t sent, u_int64_t total, const char* /*buf*/, unsigned /*len*/, void* data);

        int readJukeboxMusic( void);

        NjbMediaItem *getAlbum(const QString &artist, const QString &album);

        NjbMediaItem * getArtist(const QString &artist);

        NjbMediaItem * getDownloadAlbum(const QString &artist, const QString &album);

        NjbMediaItem * getDownloadArtist(const QString &artist);

        NjbMediaItem *addTrackToView(NjbTrack *track, NjbMediaItem *item=0);

        void clearItems();

        NjbMediaItem *m_download;

        njb_t njbs[NJB_MAX_DEVICES];

        QListView *listAmarokPlayLists;

        QString devNode;

        QString m_errMsg;

	bool m_connected;
	
        njb_t* m_njb;
        bool m_captured;
        int m_libcount;
        bool m_busy;
        trackValueList trackList;
        // playlistValueList playlistList;
        dataFileValueList dataFileList;
        // 	friend class Playlist;
        unsigned m_progressStart;
        QString m_progressMessage;

};

#endif

