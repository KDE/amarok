//
// C++ Implementation: njbmediadevice
//
// Description: This class is used to manipulate Nomad Creative Jukebox and others media player that works with the njb libraries.
//
//
// Author: Andres Oton <andres.oton@gmail.com>, (C) 2006
//
// Modified by: T.R.Shashwath <trshash84@gmail.com>
//
// Based on kionjb (http://sourceforge.net/projects/kionjb) and kzenexplorer (http://sourceforge.net/projects/kzenexplorer)
// Copyright: See COPYING file that comes with this distribution
//

#include "njbmediadevice.h"

AMAROK_EXPORT_PLUGIN( NjbMediaDevice )


// Amarok
#include <debug.h>
#include <metabundle.h>
#include <collectiondb.h>
#include <statusbar/statusbar.h>
#include <statusbar/popupMessage.h>
#include <collectiondb.h>
#include <collectionbrowser.h>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kiconloader.h>       //smallIcon
#include <kinstance.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <ktempdir.h>
#include <kurl.h>
#include <kurlrequester.h>     //downloadSelectedItems()
#include <kurlrequesterdlg.h>  //downloadSelectedItems()


// Qt
#include <qdir.h>
#include <qlistview.h>
#include <quuid.h>

// posix
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace amaroK { extern KConfig *config( const QString& ); }
njb_t *NjbMediaDevice::m_njb = 0;
// This function has NOT handled the request, so other functions may be called
// upon to do so
const int NJB_NOTHANDLED = 0;

// This function has handled the request, so no further processing is needed.
const int NJB_HANDLED = -1;


trackValueList* theTracks = 0;

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
    m_name = "NJB Media device";
    m_njb = 0;
    m_connected = false;
    m_libcount = 0;
    theTracks = &trackList;
    m_connected = false;
    m_td = 0;
    NJB_Set_Debug(0); // or try DD_SUBTRACE

}


NjbMediaDevice::~NjbMediaDevice()
{

}

bool
NjbMediaDevice::closeDevice()
{
    DEBUG_BLOCK

    if(m_connected) {
        NJB_Release( m_njb);
        m_connected = false;
    }
    m_connected = false;

    if( m_njb ) {
        NJB_Close( m_njb);

        m_njb = 0;

    }

    debug()<< "Disconnected NJB device" << endl;

    clearItems();

    return true;
}

void
NjbMediaDevice::unlockDevice()
{
}

bool
NjbMediaDevice::getCapacity(KIO::filesize_t* total, KIO::filesize_t* available)
{
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


bool
NjbMediaDevice::isConnected()
{
    return m_connected;

}

bool
NjbMediaDevice::isPlayable(const MetaBundle& bundle)
{
    DEBUG_BLOCK
            ;
    if(bundle.fileType() == MetaBundle::mp3
       || bundle.fileType() == MetaBundle::wma)
        return true;

    return false;
}

bool
NjbMediaDevice::isPreferredFormat(const MetaBundle& bundle)
{
    DEBUG_BLOCK

    if(bundle.fileType() == MetaBundle::mp3)
        return true;
    else
        return false;
}

bool
NjbMediaDevice::lockDevice(bool tryOnly)
{
    // The device is "locked" upon connection - there's very little else we can do here.
    Q_UNUSED(tryOnly);
    return true;
}

bool
NjbMediaDevice::openDevice(bool)
{
    DEBUG_BLOCK
    

    if( m_njb )
        return true;

    QString genericError = i18n( "Could not connect to Nomad device" );

    int n;
    if( NJB_Discover( njbs, 0, &n) == -1 || n == 0) {
        amaroK::StatusBar::instance()->shortLongMessage( genericError, i18n("A suitable Nomad device could not be found"), KDE::StatusBar::Error );
        debug() << ": no NJBs found\n";

        return false;
    }
    m_njb = &njbs[0];


    if( NJB_Open( m_njb ) == -1) {
        amaroK::StatusBar::instance()->shortLongMessage( genericError, i18n("Nomad device could not be opened"), KDE::StatusBar::Error );


        return false;
    }

    m_connected = true;

    if( NJB_Capture(m_njb) == -1) {
        debug() << ": couldn't capture\n";
        m_connected = false;
    }
    else
        m_connected = true;

    if( m_connected )
    {
        NJB_Set_Unicode( NJB_UC_UTF8 ); // I assume that UTF-8 is fine with everyone...
        readJukeboxMusic();
        QString s = i18n( "1 track found on device",
                          "%n tracks found on device", trackList.size() ).arg( trackList.size() );
        amaroK::StatusBar::instance()->shortMessage( s );
    }

    return true;

}

int
NjbMediaDevice::deleteFromDevice(unsigned id)
{
    int status = NJB_Delete_Track( m_njb, id );

    if( status != NJB_SUCCESS) {
        debug() << ": NJB_Delete_Track failed" << endl;
        return -1;
    }

    // remove from the cache
    trackList.remove(trackList.findTrackById( id ) );

    return 1;
}

int
NjbMediaDevice::deleteItemFromDevice(MediaItem* item, bool onlyPlayed)
{
    Q_UNUSED(onlyPlayed)
    int result = 0;
    if ( isCancelled() )
    {
        return -1;
    }

    MediaItem *next = 0;
    switch( item->type() )
    {
        case MediaItem::TRACK:
            if( isCancelled() )
                break;
            if(item)
            {
                deleteTrack( dynamic_cast<NjbMediaItem *> (item) );
                result++;
            }
            return result;
            break;
        case MediaItem::ALBUM:
        case MediaItem::ARTIST:
            // Recurse through the lists, slashing and burning.

            if( isCancelled() )
                break;

            for( MediaItem *it = dynamic_cast<MediaItem *>( item->firstChild() ); it ; it = next )
            {

                next = dynamic_cast<MediaItem *>(it->nextSibling());
                int res = deleteItemFromDevice( it, onlyPlayed );
                if( res >= 0 && result >= 0 )
                    result += res;
                else
                    result = -1;

            }
            if(item)
                delete dynamic_cast<MediaItem *>(item);
            return result;
            break;
        default:
            return 0;
            break;
    }
    return result;
}

int
NjbMediaDevice::deleteTrack(NjbMediaItem *trackItem)
{
    if(trackItem->IsDownloadItem())
    {
        delete trackItem;
        return 1;
    }

    int status = NJB_Delete_Track( m_njb, trackItem->getId());

    if( status != NJB_SUCCESS) {
        debug() << ": NJB_Delete_Track failed" << endl;
        return -1;
    }

    debug() << ": NJB_Delete_Track track deleted" << endl;

    // remove from the cache
    trackList.remove(trackList.findTrackById( trackItem->getId() ) );
    delete trackItem;
    return 1;
}

int
NjbMediaDevice::downloadSelectedItems()
{
    /* Copied from ifpmediadevice */
    QString save = QString::null;

    KURLRequesterDlg dialog( save, 0, 0 );
    dialog.setCaption( kapp->makeStdCaption( i18n( "Choose a Download Directory" ) ) );
    dialog.urlRequester()->setMode( KFile::Directory | KFile::ExistingOnly );
    dialog.exec();

    KURL destDir = dialog.selectedURL();
    if( destDir.isEmpty() )
	    return -1;

    destDir.adjustPath( 1 ); //add trailing slash
    QDir dir;
    QString path;

    QPtrList<MediaItem> items;
    m_view->getSelectedLeaves( 0, &items );
    int result = 0;

    for( MediaItem *it = items.first(); it; it = items.next() )
    {
        path = destDir.path();
        switch( it->type() ) // I really need to do first, second and third level selectors for this.
        {
            case MediaItem::ARTIST:
                path += it->bundle()->artist();
                if( !dir.exists( path ) )
                {
                    dir.mkdir( path );
                }
                break;
            case MediaItem::ALBUM:
                path += it->bundle()->artist();
                if( !dir.exists( path ) )
                {
                    dir.mkdir( path );
                }
                path += ( "/" + it->bundle()->album() );
                if( !dir.exists( path ) )
                {
                    dir.mkdir( path );
                }
                break;
            case MediaItem::TRACK:
            {
                path += it->bundle()->artist();
                if( !dir.exists( path ) )
                {
                    dir.mkdir( path );
                }
                path += ( "/" + it->bundle()->album() );
                if( !dir.exists( path ) )
                {
                    dir.mkdir( path );
                }
                NjbMediaItem *auxItem = dynamic_cast<NjbMediaItem *>( (it) );
                path +=( "/" + auxItem->getFileName() );
                if( NJB_Get_Track( m_njb, auxItem->getId(), auxItem->bundle()->filesize(), path.utf8(), progressCallback, this)
                    != NJB_SUCCESS )
                {
                    debug() << "Get Track failed. " << endl;
                    if( NJB_Error_Pending(m_njb) )
                    {
                        const char *njbError;
                        while( (njbError = NJB_Error_Geterror(m_njb) ) )
                            error() << njbError << endl;
                    }
                    else
                        debug() << "No reason to report for failure" << endl;
                }
                result ++;
                break;
            }
            default:
                break;
        }
    }
    return result;

}


/**
 * Download the selected items and put them into the collection DB.
 * @return The number of files downloaded.
*/
int
NjbMediaDevice::downloadToCollection()
{
    // We will first download all files into a temp dir, and then call move to collection.

    QPtrList<MediaItem> items;
    m_view->getSelectedLeaves( 0, &items );

    KTempDir tempdir(QString::null); // Default prefix is fine with us
    tempdir.setAutoDelete(true); // We don't need it once the work is done.
    QString path = tempdir.name(), filepath;
    KURL::List urls;
    for( MediaItem *it = items.first(); it; it = items.next() )
    {
        if( (it->type() == MediaItem::TRACK) )
        {
            NjbMediaItem* auxItem = dynamic_cast<NjbMediaItem *>( (it) );
            filepath = path + auxItem->getFileName();
            if( NJB_Get_Track( m_njb, auxItem->getId(), auxItem->bundle()->filesize(), filepath.utf8(), progressCallback, this)
                != NJB_SUCCESS )
            {
                debug() << "Get Track failed. " << endl;
                if( NJB_Error_Pending(m_njb) )
                {
                    const char *njbError;
                    while( (njbError = NJB_Error_Geterror(m_njb) ) )
                        error() << njbError << endl;
                }
                else
                    debug() << "No reason to report for failure" << endl;
            }
            urls << path+auxItem->getFileName();
        }
    }
    // Now, call the collection organizer.
    CollectionView::instance()->organizeFiles( urls, i18n( "Move Files To Collection" ), false );
    return 0;
}

MediaItem*
NjbMediaDevice::copyTrackToDevice(const MetaBundle& bundle)
{
    DEBUG_BLOCK
    debug() << "Here\n";
    trackValueList::const_iterator it_track = theTracks->findTrackByName( bundle.filename() );
    if( it_track != theTracks->end() )
    {
        deleteFromDevice( (*it_track).getId() );
    }

    // read the mp3 header
    int duration = bundle.length();
    debug() << ": URL: " << bundle.prettyURL() << endl;

    if( !duration )
    {
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
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filename( bundle.filename().utf8() ) );
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Filesize( taggedTrack->getSize() ));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Codec( taggedTrack->getCodec().utf8() ));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Title( taggedTrack->getTitle().utf8() ));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Album(taggedTrack->getAlbum().utf8()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Genre(taggedTrack->getGenre().utf8()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Artist(taggedTrack->getArtist().utf8()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Length(taggedTrack->getDuration()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Tracknum(taggedTrack->getTrackNum()));
    NJB_Songid_Addframe(songid, NJB_Songid_Frame_New_Year(taggedTrack->getYear().toUInt()));

    m_busy = true;
    debug() << ": m_njb is " << m_njb << "\n";
    kapp->processEvents( 100 );
    if(NJB_Send_Track (m_njb, bundle.url().path().utf8(), songid, progressCallback, this, &id) != NJB_SUCCESS)
    {
        debug() << ": NJB_Send_Track failed\n";
        if (NJB_Error_Pending(m_njb))
        {
            const char* njbError;
            while ((njbError = NJB_Error_Geterror(m_njb)))
                warning() << ": " << njbError << endl;
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

MediaItem*
NjbMediaDevice::newPlaylist(const QString& name, MediaItem* parent, QPtrList< MediaItem > items)
{
    DEBUG_BLOCK

    Q_UNUSED(parent);
    //MediaItem* newplaylist = new MediaItem(parent);

    NjbPlaylist playlist;
    int status = playlist.setName( name );

    if(status == NJB_SUCCESS)
    {
        //NjbMediaItem *newNjbPlayList = new NjbMediaItem(listAmarokPlayLists);
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


            //NjbMediaItem *nitem = dynamic_cast<NjbMediaItem *>(item);

        }

        status = playlist.update();
        if( status != NJB_SUCCESS)
        {
            return 0;
        }

    }

    //TODO:Crear un conversor de Playlists a las listas de Amarok
    return 0;
}

QStringList
NjbMediaDevice::supportedFiletypes()
{
    QStringList supportedFiles;
    supportedFiles << "mp3";
    supportedFiles << "wav";
    supportedFiles << "wma";
    return supportedFiles;
}

TransferDialog*
NjbMediaDevice::getTransferDialog()
{
    return m_td;
}

void
NjbMediaDevice::addConfigElements(QWidget* arg1)
{
    MediaDevice::addConfigElements(arg1);
}

void
NjbMediaDevice::addToPlaylist(MediaItem* playlist, MediaItem* after, QPtrList< MediaItem > items)
{
    MediaDevice::addToPlaylist(playlist, after, items);
}

void
NjbMediaDevice::applyConfig()
{
    MediaDevice::applyConfig();
}

void
NjbMediaDevice::cancelTransfer()
{
    MediaDevice::cancelTransfer();
}

void
NjbMediaDevice::init(MediaBrowser* parent)
{
    MediaDevice::init(parent);
}

void
NjbMediaDevice::loadConfig()
{
    MediaDevice::loadConfig();
}

void
NjbMediaDevice::removeConfigElements(QWidget* arg1)
{
    MediaDevice::removeConfigElements(arg1);
}

MediaItem *
NjbMediaDevice::trackExists( const MetaBundle & bundle )
{/*
    for( MediaItem *item = dynamic_cast<MediaItem *>( m_view->firstChild() );
         item;
         item = dynamic_cast<MediaItem *>( item->nextSibling() ) )
    {
        MetaBundle *trackBundle = new MetaBundle( ( *item->bundle() ) );
        if( trackBundle->title()     == bundle.title()
            && trackBundle->artist() == bundle.artist()
            && trackBundle->album()  == bundle.album()
            && trackBundle->track()  == bundle.track() )
        {
            return item;
        }
    }
 */
    return 0;
}

void
NjbMediaDevice::rmbPressed(QListViewItem* qitem, const QPoint& point, int )
{

    enum Actions { DOWNLOAD, DOWNLOAD_TO_COLLECTION, RENAME, DELETE};

    NjbMediaItem *item = static_cast<NjbMediaItem *>(qitem);
    if ( item )
    {
        KPopupMenu menu( m_view);
        menu.insertItem( SmallIconSet( amaroK::icon( "collection" ) ), i18n("Download file"), DOWNLOAD );
        menu.insertItem( SmallIconSet( amaroK::icon( "collection" ) ), i18n("Download to collection"), DOWNLOAD_TO_COLLECTION );
        menu.insertSeparator();
        //menu.insertItem( SmallIconSet( amaroK::icon( "edit" ) ), i18n( "Rename" ), RENAME );
        menu.insertItem( SmallIconSet( amaroK::icon( "remove" ) ), i18n( "Delete from device" ), DELETE );


        int id =  menu.exec( point );
        switch( id )
        {
        case DOWNLOAD:
            debug() << "Downloading" << endl;
            downloadSelectedItems();
            break;

        case RENAME:
            //TODO: Implement rename tracks
            //m_view->rename( item, 0 );
            break;

        case DELETE:
            MediaDevice::deleteFromDevice( item , false, true );
            break;
        case DOWNLOAD_TO_COLLECTION:
            downloadToCollection();
            break;
        }
        return;
    }

}

void
NjbMediaDevice::runTransferDialog()
{
    m_td = new TransferDialog( this );
    m_td->exec();
}

int
NjbMediaDevice::progressCallback(  u_int64_t sent, u_int64_t total, const char* /*buf*/, unsigned /*len*/, void* data)
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

    njb_media->setProgress( sent, total);

    return 0;
}

void
NjbMediaDevice::clearItems()
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
int
NjbMediaDevice::readJukeboxMusic( void)
{
    int result = NJB_SUCCESS;

    // First, read jukebox tracks
    if(trackList.isEmpty()) {

        result = trackList.readFromDevice();
    }

    if(result == NJB_SUCCESS)
    {
        clearItems();

        /*m_playlistItem = new NjbMediaItem( m_view );
        m_playlistItem->setText( 0, i18n("Playlists") );
        m_playlistItem->m_order = -5;
        m_playlistItem->setType( MediaItem::PLAYLISTSROOT );*/

        kapp->processEvents( 100 );

        trackValueList::iterator it;
        for( it = trackList.begin(); it != trackList.end(); it++)
        {

            addTrackToView( &(*it) );

            kapp->processEvents( 100 );
        }

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

njb_t *
NjbMediaDevice::theNjb()
{
    return NjbMediaDevice::m_njb;
}
