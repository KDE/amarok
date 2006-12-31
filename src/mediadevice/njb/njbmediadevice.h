/***************************************************************************
    copyright            : (C) 2006 by Andres Oton
    email                : andres.oton@gmail.com

    copyright            : (C) 2006 by T.R.Shashwath
    email                : trshash84@gmail.com
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU General Public License version 2 as    *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef NJBMEDIADEVICE_H
#define NJBMEDIADEVICE_H

#include <mediabrowser.h>
#include <transferdialog.h>
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

  Based on kionjb

  @author Andres Oton <andres.oton@gmail.com>
  @author T.R.Shashwath <trshash84@gmail.com>
 */

const int NJB_SUCCESS = 0;
const int NJB_FAILURE = -1;

// Global track list
extern trackValueList* theTracks;

class NjbMediaItem : public MediaItem
{
    public:
        NjbMediaItem( QListView *parent, QListViewItem *after = 0 ) : MediaItem( parent, after )
        {}

        NjbMediaItem( QListViewItem *parent, QListViewItem *after = 0 ) : MediaItem( parent, after )
        {}

        ~NjbMediaItem()
        {
            //m_track->removeItem(this);
        }
        
        void setTrack( NjbTrack *track ) { m_track = track; m_track->addItem(this); }
        NjbTrack *track() { return m_track; }
        QString filename() { return m_track->bundle()->url().path(); }
    private:
        NjbTrack *m_track;
};

class NjbMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
        NjbMediaDevice();

        ~NjbMediaDevice();

        virtual bool isConnected();
        virtual bool isPlayable(const MetaBundle& bundle);
        virtual bool isPreferredFormat(const MetaBundle& bundle);

        //    virtual bool needsManualConfig();

        virtual MediaItem* newPlaylist(const QString& name, MediaItem* parent, QPtrList< MediaItem > items);

        //    virtual MediaItem* tagsChanged(MediaItem* item, const MetaBundle& changed);

        virtual QStringList supportedFiletypes();

        virtual bool hasTransferDialog() { return true; }
        virtual TransferDialog* getTransferDialog();
        virtual void addConfigElements(QWidget* arg1);
        virtual void addToDirectory(MediaItem* directory, QPtrList< MediaItem > items) { Q_UNUSED(directory) Q_UNUSED(items) }
        virtual void addToPlaylist(MediaItem* playlist, MediaItem* after, QPtrList< MediaItem > items);
        virtual void applyConfig();
        virtual void init(MediaBrowser* parent);
        virtual void loadConfig();
        virtual void removeConfigElements(QWidget* arg1);
        virtual void rmbPressed(QListViewItem* qitem, const QPoint& point, int arg1);
        virtual void runTransferDialog();
        virtual void customClicked();

        void setDeviceType(const QString& type);
        void setSpacesToUnderscores(bool yesno);
        static njb_t *theNjb();
    public slots:
        void expandItem( QListViewItem *item );
        
    protected:

        virtual bool closeDevice();
        virtual bool getCapacity(KIO::filesize_t* total, KIO::filesize_t* available);

        // virtual bool isSpecialItem(MediaItem* item);

        virtual bool lockDevice(bool tryOnly);
        virtual bool openDevice(bool silent);

        int deleteFromDevice(unsigned id);
        int deleteItemFromDevice(MediaItem* item, int flags=DeleteTrack );
        int deleteTrack(NjbMediaItem *trackItem);

        int downloadSelectedItems();
        int downloadToCollection();

        virtual MediaItem* copyTrackToDevice(const MetaBundle& bundle);
        virtual void copyTrackFromDevice(MediaItem *item);

        virtual void cancelTransfer();
        virtual void synchronizeDevice() {};

        virtual void unlockDevice();

        virtual void updateRootItems() {};
        
    private:
        // TODO:
        MediaItem        *trackExists( const MetaBundle& );
        // miscellaneous methods
        static int progressCallback( u_int64_t sent, u_int64_t total, const char* /*buf*/, unsigned /*len*/, void* data);

        int readJukeboxMusic( void);
        void clearItems();
        
        NjbMediaItem *addTrackToView(NjbTrack *track, NjbMediaItem *item=0);
        NjbMediaItem* addAlbums( const QString &artist, NjbMediaItem *item );
        NjbMediaItem* addTracks( const QString &artist, const QString &track, NjbMediaItem *item );
        NjbMediaItem* addTrack( NjbTrack *track );
        NjbMediaItem* addArtist( NjbTrack *track );
        TransferDialog      *m_td;
        
        QListView *listAmarokPlayLists;
        QString devNode;
        QString m_errMsg;
        bool m_connected; // Replaces m_captured from the original code.

        njb_t njbs[NJB_MAX_DEVICES];
        static njb_t* m_njb;
        trackValueList trackList;
        
        int m_libcount;
        bool m_busy;
        unsigned m_progressStart;
        QString m_progressMessage;
        NjbMediaItem *m_artistItem;
        NjbMediaItem *m_albumItem;
        NjbMediaItem *m_allTracksItem;
};

#endif

