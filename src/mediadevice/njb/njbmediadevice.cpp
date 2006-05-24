//
// C++ Implementation: njbmediadevice
//
// Description: This class is used to manipulate Nomad Creative Jukebox and others media player that works with the njb libraries. 
//
//
// Author: Andres Oton <andres.oton@gmail.com>, (C) 2006
//
// Based at kionjb (http://sourceforge.net/projects/kionjb)
// Copyright: See COPYING file that comes with this distribution
//
//
#include "njbmediadevice.h"

AMAROK_EXPORT_PLUGIN( NjbMediaDevice )

#include "debug.h"
#include "metabundle.h"
#include "collectiondb.h"
#include "statusbar/statusbar.h"
#include "statusbar/popupMessage.h"

#include <quuid.h>

// kde
#include <kapplication.h>
#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kurl.h>
#include <kiconloader.h>       //smallIcon
#include <kpopupmenu.h>
#include <kfiledialog.h>
#include <qlistview.h>
#include <qdir.h>

// posix
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace amaroK { extern KConfig *config( const QString& ); }

// This function has NOT handled the request, so other functions may be called
// upon to do so
const int NJB_NOTHANDLED = 0;

// This function has handled the request, so no further processing is needed.
const int NJB_HANDLED = -1;

njb_t* theNjb = NULL;
trackValueList* theTracks = NULL;

class NjbMediaItem : public MediaItem
{
    public:
        NjbMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        {isdownload = false;}

        NjbMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        {isdownload=false;}

        void setId(unsigned id) {trid=id;}

        unsigned getId() {return trid;}

        QString getFileName() {return filename;}

        void setFileName(QString name) {filename = name;}

        void setIsDownloadItem(bool d) {isdownload = d;}

        bool IsDownloadItem() {return isdownload;}

    private:

        unsigned trid;
        QString filename;
        bool isdownload;
};


NjbMediaDevice::NjbMediaDevice(): MediaDevice()
{

    //	listAmarokPlayLists = new QListView();

    theNjb = m_njb = NULL;
    m_captured = false;
    m_libcount = 0;
    theTracks = &trackList;
    m_connected = false;

    NJB_Set_Debug(0); // or try DD_SUBTRACE

}


NjbMediaDevice::~NjbMediaDevice()
{

}


// bool NjbMediaDevice::configBool(const QString& name, bool defValue)
// {
//     return MediaDevice::configBool(name, defValue);
// }
// 
// bool NjbMediaDevice::getSpacesToUnderscores()
// {
//     return MediaDevice::getSpacesToUnderscores();
// }
// 
// bool NjbMediaDevice::isCancelled()
// {
//     return MediaDevice::isCancelled();
// }
// 
// bool NjbMediaDevice::isDeleting()
// {
//     return MediaDevice::isDeleting();
// }
// 
// bool NjbMediaDevice::isTransferring()
// {
//     return MediaDevice::isTransferring();
// }

// int NjbMediaDevice::progress() const
// {
//     return MediaDevice::progress();
// }
// 
// MediaItem* NjbMediaDevice::transferredItem()
// {
//     return MediaDevice::transferredItem();
// }
// 
// MediaView* NjbMediaDevice::view()
// {
//     return MediaDevice::view();
// }
// 
// Medium* NjbMediaDevice::getMedium()
// {
//     return MediaDevice::getMedium();
// }
// 
// QString NjbMediaDevice::configString(const QString& name, const QString& defValue)
// {
//     return MediaDevice::configString(name, defValue);
// }

// QString NjbMediaDevice::deviceNode() const
// {
// 	return devNode;
// }

// QString NjbMediaDevice::deviceType()
// {
//     return MediaDevice::deviceType();
// }

// QString NjbMediaDevice::getTransferDir()
// {
//     return MediaDevice::getTransferDir();
// }

// QString NjbMediaDevice::name() const
// {
//     return MediaDevice::name();
// }
// 
// QString NjbMediaDevice::uniqueId() const
// {
//     return MediaDevice::uniqueId();
// }
// 
// bool NjbMediaDevice::asynchronousTransfer()
// {
//     return MediaDevice::asynchronousTransfer();
// }

// bool NjbMediaDevice::autoConnect()
// {
//     return MediaDevice::autoConnect();
// }

bool NjbMediaDevice::closeDevice()
{
    DEBUG_BLOCK

    if(m_captured) {
        NJB_Release( m_njb);
        m_captured = false;
    }
    m_connected = false;

    if( m_njb ) {
        debug() << ": disconnecting. Is captured: "<< m_captured << endl;
        /*		if(m_captured)
                        unlockDevice();*/

        NJB_Close( m_njb);
        debug() << ": deleting m_njb " << m_njb << endl;
        delete m_njb;
        m_njb = NULL;
        theNjb = m_njb;
    }

    debug()<< ": disconnected, pid=" << getpid() << endl;

    clearItems();

    return true;
}

void NjbMediaDevice::unlockDevice()
{
    DEBUG_BLOCK
}

bool NjbMediaDevice::getCapacity(KIO::filesize_t* total, KIO::filesize_t* available)
{
    DEBUG_BLOCK

    if(!m_connected)
	return false;


    u_int64_t itotal;
    u_int64_t ifree;

    if(NJB_Get_Disk_Usage(m_njb, &itotal, &ifree) == -1)
	return false;

    *total = itotal;
    *available = ifree;

    return true;

}

// bool NjbMediaDevice::hasTransferDialog()
// {
//     return MediaDevice::hasTransferDialog();
// }

//DONE
bool NjbMediaDevice::isConnected()
{
    return m_connected;

}

bool NjbMediaDevice::isPlayable(const MetaBundle& bundle)
{
    DEBUG_BLOCK
    debug() << ": pid=" << getpid() << endl;
    if(bundle.fileType() == MetaBundle::mp3)
        return true;

    return false;
}

bool NjbMediaDevice::isPreferredFormat(const MetaBundle& bundle)
{
    DEBUG_BLOCK
    debug() << ": pid=" << getpid() << endl;
    if(bundle.fileType() == MetaBundle::mp3)
        return true;
    else
        return false;
}

// bool NjbMediaDevice::isSpecialItem(MediaItem* item)
// {
//     return MediaDevice::isSpecialItem(item);
// }

bool NjbMediaDevice::lockDevice(bool tryOnly)
{
    DEBUG_BLOCK
    debug() << ": pid=" << getpid() << endl;

    return true;
}

//bool NjbMediaDevice::needsManualConfig()
//{
//    return MediaDevice::needsManualConfig();
//}


//DONE
bool NjbMediaDevice::openDevice(bool)
{
    DEBUG_BLOCK
    debug() << ": pid=" << getpid() << endl;

    if( m_njb )
        return true;

    QString genericError = i18n( "Could not connect to Nomad device" );

    int n;
    if( NJB_Discover( njbs, 0, &n) == -1 || n == 0) {
        amaroK::StatusBar::instance()->shortLongMessage( genericError, i18n("Nomad: Connecting "), KDE::StatusBar::Error );
        debug() << ": no NJBs found\n";
        theNjb = m_njb = NULL;
        return false;
    }

    m_njb = new njb_t;
    *m_njb = njbs[0];
    theNjb = m_njb;

    if( NJB_Open( m_njb) == -1) {
        debug() << ": couldn't open\n";
        debug() << ": deleting " << m_njb << "\n";
        delete m_njb;
        theNjb = m_njb = NULL;
        return false;
    }
    m_connected = true;

    debug() << ": m_njb " << m_njb << "\n";

    debug() << ": pid=" << getpid() << "Capturing." << endl;

    if( NJB_Capture(m_njb) == -1) {
        debug() << ": couldn't capture\n";
        m_captured = false;
    }
    else
        m_captured = true;

    if(m_captured)
    {
        readJukeboxMusic( );
	QString s;
	s.sprintf("%d tracks found", trackList.size());
        amaroK::StatusBar::instance()->shortLongMessage( s, s, KDE::StatusBar::Information );
    }

    return true;

}

int NjbMediaDevice::deleteFromDevice(unsigned id)
{
    debug() << ": pid=" << getpid() << endl;

    if(lockDevice( false) )
        return -1;

    int status = NJB_Delete_Track( m_njb, id );

    if( status != NJB_SUCCESS) {
        debug() << ": NJB_Delete_Track failed" << endl;
        return -1;
    }

    // remove from the cache
    trackList.remove(trackList.findTrackById( id ) );

    return 1;
}

int NjbMediaDevice::deleteItemFromDevice(MediaItem* item, bool onlyPlayed)
{
    Q_UNUSED(onlyPlayed)
    //onlyPlayed is ignored because this device doesn't suppor directories 
    //and not store the number of times that a file is played

    debug() << endl;

    NjbMediaItem *njbItem = dynamic_cast<NjbMediaItem *>(item);
    if(item->type() == MediaItem::ARTIST)
        return deleteArtist(njbItem);

    if(item->type() == MediaItem::ALBUM)
        return deleteAlbum(njbItem);

    if(item->type() == MediaItem::TRACK)
        return deleteTrack( njbItem );

    debug() << ": OK" << endl;

    return -1;
}

int NjbMediaDevice::deleteArtist(NjbMediaItem *artistItem)
{
    debug() << endl;
    if(artistItem->IsDownloadItem())
    {
        delete artistItem;
        return 1;
    }

    int itemsDeleted = 0;
    if(artistItem->type() == MediaItem::ARTIST)
    {
        NjbMediaItem* auxItem = dynamic_cast<NjbMediaItem *>(artistItem->firstChild());
        while(auxItem)
        {
            itemsDeleted += deleteAlbum( auxItem );
            auxItem = dynamic_cast<NjbMediaItem *>(auxItem->nextSibling());
        }
    }
    delete artistItem;
    return itemsDeleted;
}

int NjbMediaDevice::deleteAlbum(NjbMediaItem *albumItem)
{
    debug() << endl;

    if(albumItem->IsDownloadItem())
    {
        delete albumItem;
        return 1;
    }
    int itemsDeleted = 0;
    if(albumItem->type() == MediaItem::ALBUM)
    {
        NjbMediaItem *auxItem = dynamic_cast<NjbMediaItem *>(albumItem->firstChild());
        while(auxItem)
        {
            itemsDeleted += deleteTrack(auxItem);
            auxItem = dynamic_cast<NjbMediaItem *>(auxItem->nextSibling());
        }
    }
    delete albumItem;
    return itemsDeleted;
}

int NjbMediaDevice::deleteTrack(NjbMediaItem *trackItem)
{
    debug() << endl;

    if(trackItem->IsDownloadItem())
    {
        delete trackItem;
        return 1;
    }

    if(!lockDevice( false ) )
        return -1;

    int status = NJB_Delete_Track( m_njb, trackItem->getId());

    if( status != NJB_SUCCESS) {
        debug() << ": NJB_Delete_Track failed" << endl;
        return -1;
    }

    debug() << ": NJB_Delete_Track track deleted" << endl;

    // remove from the cache
    trackList.remove(trackList.findTrackById( trackItem->getId() ) );

    unlockDevice();

    //	readJukeboxMusic();
    delete trackItem;
    return 1;
}

int NjbMediaDevice::downloadSelectedItems( NjbMediaItem * item )
{
    debug() << endl;

    NjbMediaItem *njbItem = dynamic_cast<NjbMediaItem *>(item);
    if(item->type() == MediaItem::ARTIST)
        return downloadArtist(njbItem);

    if(item->type() == MediaItem::ALBUM)
        return downloadAlbum(njbItem);

    if(item->type() == MediaItem::TRACK)
        return downloadTrack( njbItem );

    debug() << ": OK" << endl;

    return -1;

}

int NjbMediaDevice::downloadArtist(NjbMediaItem *artistItem)
{
    debug() << endl;

    int itemsDownload = 0;

    if(artistItem->type() == MediaItem::ARTIST)
    {
        NjbMediaItem *dArtist = getDownloadArtist( artistItem->text(0) );

        NjbMediaItem* auxItem = dynamic_cast<NjbMediaItem *>(artistItem->firstChild());

        while(auxItem)
        {
            itemsDownload += downloadAlbum( auxItem );
            auxItem = dynamic_cast<NjbMediaItem *>(auxItem->nextSibling());
        }
    }
    return itemsDownload;
}

int NjbMediaDevice::downloadAlbum(NjbMediaItem *albumItem)
{
    debug() << endl;

    int itemsDownload = 0;

    if(albumItem->type() == MediaItem::ALBUM)
    {
        NjbMediaItem *dAlbum = getDownloadAlbum(albumItem->parent()->text(0),albumItem->text(0));

        NjbMediaItem* auxItem = dynamic_cast<NjbMediaItem *>(albumItem->firstChild());

        while(auxItem)
        {
            itemsDownload += downloadTrack( auxItem );
            auxItem = dynamic_cast<NjbMediaItem *>(auxItem->nextSibling());
        }
    }
    return itemsDownload;
}

int NjbMediaDevice::downloadTrack(NjbMediaItem *trackItem)
{
    debug() << endl;

    if(trackItem->type() == MediaItem::TRACK)
    {
        debug() << "Artist: " << trackItem->parent()->parent()->text(0) << "  Album: " << trackItem->parent()->text(0) << endl;
        NjbMediaItem *albumItem = getDownloadAlbum(trackItem->parent()->parent()->text(0),trackItem->parent()->text(0));
        NjbMediaItem *auxItem = dynamic_cast<NjbMediaItem *>( albumItem->firstChild() );
        while(auxItem)
        {
            if(auxItem->text(0)==trackItem->text(0))
                return 0;
            auxItem = dynamic_cast<NjbMediaItem *>( auxItem->nextSibling() );
        }

        NjbMediaItem *newDownload = new NjbMediaItem(albumItem);
        newDownload->setText(0,trackItem->text(0));
        newDownload->setType(MediaItem::TRACK);
        newDownload->setId(trackItem->getId());
        MetaBundle *newBundle = new MetaBundle();
        newBundle->copyFrom( *(trackItem->bundle()) );
        newDownload->setBundle( newBundle );
        newDownload->setFileName( trackItem->getFileName());
        newDownload->setIsDownloadItem( true );

        return 1;
    }
    return 0;
}

int NjbMediaDevice::downloadNow()
{
    debug() << endl;
    int dtracks = 0;
    QString path = KFileDialog::getExistingDirectory(  );

    if(path.isEmpty())
        return 0;

    NjbMediaItem *auxItem = dynamic_cast<NjbMediaItem *>( m_download->firstChild() );
    QListViewItemIterator it(m_download);
    while(it.current())
    {
        NjbMediaItem *item = dynamic_cast<NjbMediaItem *>(it.current());
        if(item->type() == MediaItem::TRACK && item->IsDownloadItem())
        {
            dtracks += downloadTrackNow(item, path);
            if(this->isCancelled())
                break;
        }
        ++it;
    }

    this->setCancelled( false );
    this->hideProgress();

    return dtracks;
}

int NjbMediaDevice::downloadTrackNow( NjbMediaItem *item , QString path)
{
    debug() << endl;

    if(!lockDevice(false))
        return 0;

    //int   NJB_Get_Track (njb_t *njb, u_int32_t trackid, u_int32_t size, const char *path, NJB_Xfer_Callback *callback, void *data)
    QString file = path + "/";
    file += item->bundle()->artist();
    debug() << "Getting track: "<< file << endl;
    QDir qdir;
    qdir.mkdir(file);
    file = file + "/" + item->bundle()->album();
    qdir.mkdir(file);
    file = file + "/" + item->getFileName();
    if(NJB_Get_Track (m_njb, item->getId(), item->bundle()->filesize(), file.latin1(), progressCallback, this) != NJB_SUCCESS)
    {
        debug() << ": NJB_Send_Track failed\n";
        if (NJB_Error_Pending(m_njb))
        {
            const char* error;
            while ((error = NJB_Error_Geterror(m_njb)))
                warning() << ": " << error << endl;
        }
        else
            debug() << ": No reason for failure reported.\n";

        m_busy = false;

        return 0;
    }

    return 1;
}

MediaItem* NjbMediaDevice::copyTrackToDevice(const MetaBundle& bundle)
{
    //TODO:REVIEW
    debug() << ": pid=" << getpid() << endl;

    if(!lockDevice( false) )
        return 0;

    trackValueList::const_iterator it_track = theTracks->findTrackByName( bundle.filename() );
    if( it_track != theTracks->end() ) {
        deleteFromDevice( (*it_track).getId() );
    }

    if(!lockDevice( false) )
        return 0;

    // read the mp3 header
    int duration = bundle.length();
    debug() << ": URL: " << bundle.prettyURL() << endl;

    if( !duration ) {
        m_errMsg = i18n( "Not a valid mp3 file");
        return 0;
    }

    NjbTrack *taggedTrack = new NjbTrack();
    taggedTrack->setSize( bundle.filesize() );
    taggedTrack->setDuration( bundle.length() );
    taggedTrack->setFilename( bundle.filename() );
    taggedTrack->setTitle( bundle.title() );
    taggedTrack->setGenre( bundle.genre() );
    taggedTrack->setArtist( bundle.artist() );
    taggedTrack->setAlbum( bundle.album() );
    taggedTrack->setCodec( "mp3" );
    taggedTrack->setTrackNum( bundle.track() );
    taggedTrack->setYear( QString::number( bundle.year() ) );

    // send the track
    // totalSize( taggedTrack.getSize() );
    debug() << "copyTrack: sending..." << endl;
    debug() << "copyTrack: "
        << taggedTrack->getTitle() << " " << taggedTrack->getAlbum() << " "
        << taggedTrack->getGenre() << " " 
        << "size:" << taggedTrack->getSize() << " " 
        << taggedTrack->getArtist() << endl;

    u_int32_t id;
    m_progressStart = time( 0);
    m_progressMessage = "Copying / Sent %1%...";

    njb_songid_t* songid = NJB_Songid_New();
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filename( bundle.filename().latin1() ) );
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filesize( taggedTrack->getSize() ));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Codec( taggedTrack->getCodec().latin1() ));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Title( taggedTrack->getTitle().latin1() ));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Album(taggedTrack->getAlbum().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Genre(taggedTrack->getGenre().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Artist(taggedTrack->getArtist().latin1()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Length(taggedTrack->getDuration()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Tracknum(taggedTrack->getTrackNum()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Year(taggedTrack->getYear().toUInt()));

    m_busy = true;
    debug() << ": m_njb is " << m_njb << "\n";
    kapp->processEvents( 100 );
    if(NJB_Send_Track (m_njb, bundle.url().path().latin1(), songid, progressCallback, this, &id) != NJB_SUCCESS)
    {
        debug() << ": NJB_Send_Track failed\n";
        if (NJB_Error_Pending(m_njb)) 
        {
            const char* error;
            while ((error = NJB_Error_Geterror(m_njb)))
                warning() << ": " << error << endl;
        }
        else
            debug() << ": No reason for failure reported.\n";

        m_busy = false;
        NJB_Songid_Destroy( songid );

        return 0;
    }

    m_busy = false;
    NJB_Songid_Destroy( songid );

    taggedTrack->setId( id );

    // cache the track
    trackList.append( *taggedTrack );;

    //TODO: Construct a MediaItem

    return addTrackToView( taggedTrack );

}

MediaItem* NjbMediaDevice::newPlaylist(const QString& name, MediaItem* parent, QPtrList< MediaItem > items)
{
    DEBUG_BLOCK
    debug() << ": pid=" << getpid() << endl;

    MediaItem* newplaylist = new MediaItem(parent);

    NjbPlaylist playlist;
    int status = playlist.setName( name );

    if(status == NJB_SUCCESS)
    {
        NjbMediaItem *newNjbPlayList = new NjbMediaItem(listAmarokPlayLists);
        for(MediaItem *item=items.first();item;item=items.next())
        {

            status = playlist.addTrack( item->bundle()->filename() );
            if( status == NJB_FAILURE)
            {
                //TODO: Show a message with the error.
            }
            else if( status != NJB_SUCCESS)
            {
                return 0;
            }


            NjbMediaItem *nitem = dynamic_cast<NjbMediaItem *>(item);

        }

        status = playlist.update();
        if( status != NJB_SUCCESS)
        {
            return 0;
        }

        // update cache
        // playlistList.readFromDevice();

        // listPlayLists.add(playlist);
    }

    //TODO:Crear un conversor de Playlists a las listas de Amarok
    return 0;	
    // return MediaDevice::newPlaylist(name, parent, items);
}

// MediaItem* NjbMediaDevice::tagsChanged(MediaItem* item, const MetaBundle& changed)
// {
//    return MediaDevice::tagsChanged(item, changed);
// }

QStringList NjbMediaDevice::supportedFiletypes()
{
    return MediaDevice::supportedFiletypes();
}

TransferDialog* NjbMediaDevice::getTransferDialog()
{
    return MediaDevice::getTransferDialog();
}

void NjbMediaDevice::addConfigElements(QWidget* arg1)
{
    MediaDevice::addConfigElements(arg1);
}

void NjbMediaDevice::addToDirectory(MediaItem* directory, QPtrList< MediaItem > items)
{
    MediaDevice::addToDirectory(directory, items);
}

void NjbMediaDevice::addToPlaylist(MediaItem* playlist, MediaItem* after, QPtrList< MediaItem > items)
{
    MediaDevice::addToPlaylist(playlist, after, items);
}

void NjbMediaDevice::applyConfig()
{
    MediaDevice::applyConfig();
}

void NjbMediaDevice::cancelTransfer()
{
    MediaDevice::cancelTransfer();
}

// DONE
void NjbMediaDevice::init(MediaBrowser* parent)
{
    MediaDevice::init(parent);
}

void NjbMediaDevice::loadConfig()
{
    MediaDevice::loadConfig();
}

void NjbMediaDevice::removeConfigElements(QWidget* arg1)
{
    MediaDevice::removeConfigElements(arg1);
}

void NjbMediaDevice::rmbPressed(QListViewItem* qitem, const QPoint& point, int )
{

    enum Actions { DOWNLOAD, RENAME, DELETE, DOWNLOADNOW };

    NjbMediaItem *item = static_cast<NjbMediaItem *>(qitem);
    if ( item )
    {
        KPopupMenu menu(m_view);
        if(item == m_download)
        {
            menu.insertItem( SmallIconSet( "down" ), i18n( "Download Now!" ), DOWNLOADNOW );
        }
        else
        {
            menu.insertItem(SmallIconSet("down"), i18n("Download"), DOWNLOAD);
            menu.insertSeparator();
            //			menu.insertItem( SmallIconSet( "editclear" ), i18n( "Rename" ), RENAME );
            menu.insertItem( SmallIconSet( "editdelete" ), i18n( "Delete" ), DELETE );
        }

        int id =  menu.exec( point );
        switch( id )
        {
        case DOWNLOADNOW:
            downloadNow();
            break;

        case DOWNLOAD:
            downloadSelectedItems( item );
            break;

        case RENAME:
            //TODO: Implement rename tracks
            //m_view->rename( item, 0 );
            break;

        case DELETE:
            deleteItemFromDevice( item , false);
            break;
        }
        return;
    }

}

void NjbMediaDevice::runTransferDialog()
{
    MediaDevice::runTransferDialog();
}

void NjbMediaDevice::synchronizeDevice()
{
}

void NjbMediaDevice::updateRootItems()
{
    MediaDevice::updateRootItems();
}

void NjbMediaDevice::hideProgress()
{
    MediaDevice::hideProgress();
}

void NjbMediaDevice::purgeEmptyItems(MediaItem* root)
{
    MediaDevice::purgeEmptyItems(root);
}

void NjbMediaDevice::setConfigBool(const QString& name, bool value)
{
    MediaDevice::setConfigBool(name, value);
}

void NjbMediaDevice::setConfigString(const QString& name, const QString& value)
{
    MediaDevice::setConfigString(name, value);
}

void NjbMediaDevice::setDeviceType(const QString& type)
{
    MediaDevice::setDeviceType(type);
}

void NjbMediaDevice::setFirstSort(QString text)
{
    MediaDevice::setFirstSort(text);
}

void NjbMediaDevice::setSecondSort(QString text)
{
    MediaDevice::setSecondSort(text);
}

void NjbMediaDevice::setSpacesToUnderscores(bool yesno)
{
    MediaDevice::setSpacesToUnderscores(yesno);
}

void NjbMediaDevice::setThirdSort(QString text)
{
    MediaDevice::setThirdSort(text);
}

void NjbMediaDevice::syncStatsFromDevice(MediaItem* root)
{
    MediaDevice::syncStatsFromDevice(root);
}

void NjbMediaDevice::syncStatsToDevice(MediaItem* root)
{
    MediaDevice::syncStatsToDevice(root);
}

int NjbMediaDevice::progressCallback(  u_int64_t sent, u_int64_t total, const char* /*buf*/, unsigned /*len*/, void* data)
{
    kapp->processEvents( 100 );

    NjbMediaDevice *njb_media = reinterpret_cast<NjbMediaDevice*>(data);

    if( njb_media->isCancelled() )
    {
        debug() << "Cancelling transfer operation" << endl;
        njb_media->setCancelled( false );
        njb_media->setProgress( sent, total );
        return 1;
    }

    debug() << ": pid=" << getpid() <<  " setProgress " << endl;
    njb_media->setProgress( sent, total);

    return 0;
}

void NjbMediaDevice::clearItems()
{
    NjbMediaItem *auxItem = dynamic_cast<NjbMediaItem *>( m_view->firstChild() );
    while(auxItem)
    {
        NjbMediaItem *nextItem = dynamic_cast<NjbMediaItem *>(auxItem->nextSibling());
        delete auxItem;
        auxItem = nextItem;
    }
}

/* ------------------------------------------------------------------------ */
/** Transfer musical info from the njb to local structures */
int NjbMediaDevice::readJukeboxMusic( void)
{
    int result = NJB_SUCCESS;

    // First, read jukebox tracks
    if(trackList.isEmpty()) {

        result = trackList.readFromDevice();
    }

    if(result == NJB_SUCCESS)
    {
        clearItems();

        m_playlistItem = new NjbMediaItem( m_view );
        m_playlistItem->setText( 0, i18n("Playlists") );
        m_playlistItem->m_order = -5;
        m_playlistItem->setType( MediaItem::PLAYLISTSROOT );

        m_download = new NjbMediaItem( m_view );
        m_download->setText( 0, i18n("Download"));
        m_download->setType( MediaItem::DIRECTORY );
        m_download->m_order = -5;

        // 		m_invisibleItem = new NjbMediaItem( m_view );
        // 		m_invisibleItem->setText( 0, i18n("Invisible") );
        // 		m_invisibleItem->m_order = -3;
        // 		m_invisibleItem->setType( MediaItem::INVISIBLEROOT );
        //
        // 		m_staleItem = new NjbMediaItem( m_view );
        // 		m_staleItem->setText( 0, i18n("Stale") );
        // 		m_staleItem->m_order = -2;
        // 		m_staleItem->setType( MediaItem::STALEROOT );
        //
        // 		m_orphanedItem = new NjbMediaItem( m_view );
        // 		m_orphanedItem->setText( 0, i18n("Orphaned") );
        // 		m_orphanedItem->m_order = -2;
        // 		m_orphanedItem->setType( MediaItem::ORPHANEDROOT );

        kapp->processEvents( 100 );

        trackValueList::iterator it;
        for( it = trackList.begin(); it != trackList.end(); it++)
        {
            //listEntry( createUDSEntry( url, ( *it).getFilename()), false);
            // debug() << "Adding track to m_view : " << (*it).getFilename() << endl;

            addTrackToView( &(*it) );

            /*			NjbMediaItem *newItem = new NjbMediaItem(m_view);
                                newItem->setType( MediaItem::TRACK );
                                newItem->setText(0,(*it).getFilename());
                                newItem->setBundle( (*it).getMetaBundle() );
                                newItem->m_device = this;*/
            kapp->processEvents( 100 );
        }

        // Then read jukebox playlists
        //if ( result == NJB_SUCCESS )
        //	result = playlistList.readFromDevice();

        // After loading, disconnect.  This will give the player a chance to
        // catch up.  Otherwise we'll start trying to write while it's
        // still working on the track database, which seems to cause hard
        // system freezes

        unlockDevice();
    }


    debug() << ": return " << result << endl;
    return result;
}

NjbMediaItem *
NjbMediaDevice::addTrackToView(NjbTrack *track, NjbMediaItem *item)
{
    QString artistName;
    artistName = track->getArtist();

    NjbMediaItem *artist = getArtist(artistName);
    if(!artist)
    {
        artist = new NjbMediaItem(m_view);
        artist->setText( 0, artistName );
        artist->setType( MediaItem::ARTIST );
    }

    QString albumName = track->getAlbum();
    NjbMediaItem *album = (NjbMediaItem*)artist->findItem(albumName);
    if(!album)
    {
        album = new NjbMediaItem( artist );
        album->setText( 0, albumName );
        album->setType( MediaItem::ALBUM );
    }

    if( item )
        album->insertItem( item );
    else
    {
        item = new NjbMediaItem( album );

        QString titleName = track->getTitle();

        item->setText( 0, titleName );
        item->setType( MediaItem::TRACK );
        item->setBundle( track->getMetaBundle());
        item->setId( track->getId() );
        item->setFileName( track->getFilename() );
    }
    return item;
    //	item->m_order = track->track_nr;

}

NjbMediaItem *
NjbMediaDevice::getArtist(const QString &artist)
{
    for(NjbMediaItem *it = dynamic_cast<NjbMediaItem *>(m_view->firstChild());
            it;
            it = dynamic_cast<NjbMediaItem *>(it->nextSibling()))
    {
        if(it->m_type==MediaItem::ARTIST && artist == it->text(0))
            return it;
    }

    return 0;
}

NjbMediaItem *
NjbMediaDevice::getAlbum(const QString &artist, const QString &album)
{
    NjbMediaItem *item = getArtist(artist);
    if(item)
        return dynamic_cast<NjbMediaItem *>(item->findItem(album));

    return 0;
}

NjbMediaItem * NjbMediaDevice::getDownloadArtist(const QString &artist)
{
    debug() << " Artist: " << artist << endl;

    for(NjbMediaItem *it = dynamic_cast<NjbMediaItem *>(m_download->firstChild());
            it;
            it = dynamic_cast<NjbMediaItem *>(it->nextSibling()))
    {
        if(it->m_type==MediaItem::ARTIST && artist == it->text(0))
            return it;
    }

    debug() << " Creating new Artist Download" << endl;

    NjbMediaItem *dArtist = new NjbMediaItem(m_download);
    dArtist->setText(0, artist );
    dArtist->setType( MediaItem::ARTIST );

    debug() << " End" << endl;

    return dArtist;
}

NjbMediaItem * NjbMediaDevice::getDownloadAlbum(const QString &artist, const QString &album)
{
    debug() << "Artist: " << artist << "  Album:" << album << endl;

    NjbMediaItem *item = getDownloadArtist(artist);

    NjbMediaItem *dAlbum = 0;

    if(item)
        dAlbum = dynamic_cast<NjbMediaItem *>(item->findItem(album));

    if(!dAlbum)
    {
        debug() << "Creating new Album Download" << endl;

        dAlbum = new NjbMediaItem( getDownloadArtist( artist ) );
        dAlbum->setText(0, album);
        dAlbum->setType( MediaItem::ALBUM );
    }
    return dAlbum;
}
