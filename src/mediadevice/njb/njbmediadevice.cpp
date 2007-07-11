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

#include "njbmediadevice.h"

AMAROK_EXPORT_PLUGIN( NjbMediaDevice )


// Amarok
#include <collectiondb.h>
#include <collectionbrowser.h>
#include <debug.h>
#include <metabundle.h>
#include <statusbar/statusbar.h>
#include <statusbar/popupMessage.h>


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
#include <ktoolbarbutton.h>
#include <kurl.h>
#include <kurlrequester.h>     //downloadSelectedItems()
#include <kurlrequesterdlg.h>  //downloadSelectedItems()


// Qt
#include <qdir.h>
#include <qlistview.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <quuid.h>

// posix
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace Amarok { extern KConfig *config( const QString& ); }
njb_t *NjbMediaDevice::m_njb = 0;
// This function has NOT handled the request, so other functions may be called
// upon to do so
const int NJB_NOTHANDLED = 0;

// This function has handled the request, so no further processing is needed.
const int NJB_HANDLED = -1;

NjbMediaDevice::NjbMediaDevice(): MediaDevice()
{

    //	listAmarokPlayLists = new QListView();
    m_name = i18n("NJB Media device");
    m_njb = 0;
    m_connected = false;
    m_libcount = 0;
    m_connected = false;
    m_customButton = true;
    m_td = 0;
    NJB_Set_Debug(0); // or try DD_SUBTRACE
    KToolBarButton* customButton = MediaBrowser::instance()->getToolBar()->getButton( MediaBrowser::CUSTOM );
    customButton->setText( i18n("Special device functions") );
    QToolTip::remove( customButton );
    QToolTip::add( customButton, i18n( "Special functions of your jukebox" ) );
}


NjbMediaDevice::~NjbMediaDevice()
{

}

bool
NjbMediaDevice::closeDevice()
{
    DEBUG_BLOCK

    if(m_connected)
    {
        NJB_Release( m_njb);
        m_connected = false;
    }
    m_connected = false;

    if( m_njb )
    {
        NJB_Close( m_njb);

        m_njb = 0;

    }

    debug()<< "Disconnected NJB device" << endl;

    clearItems();

    m_name = i18n("NJB Media device");
    debug() << "Done" << endl;
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
    NJB_Set_Unicode( NJB_UC_UTF8 ); // I assume that UTF-8 is fine with everyone...

    int n;
    if( NJB_Discover( njbs, 0, &n) == -1 || n == 0 )
    {
        Amarok::StatusBar::instance()->shortLongMessage( genericError, i18n("A suitable Nomad device could not be found"), KDE::StatusBar::Error );
        debug() << ": no NJBs found\n";

        return false;
    }
    m_njb = &njbs[0];


    if( NJB_Open( m_njb ) == -1 )
    {
        Amarok::StatusBar::instance()->shortLongMessage( genericError, i18n("Nomad device could not be opened"), KDE::StatusBar::Error );


        return false;
    }

    QString deviceName = NJB_Get_Device_Name( m_njb, 1 );
    QString owner = NJB_Get_Owner_String( m_njb );
    m_name = deviceName + " (Owned by " + owner + ')';


    if( NJB_Capture(m_njb) == -1)
    {
        debug() << ": couldn't capture\n";
        m_connected = false;
    }
    else
    {
        m_connected = true;
        readJukeboxMusic();
    }

    return true;

}

int
NjbMediaDevice::deleteFromDevice(unsigned id)
{
    int status = NJB_Delete_Track( m_njb, id );

    if( status != NJB_SUCCESS)
    {
        debug() << ": NJB_Delete_Track failed" << endl;
        return -1;
    }

    // remove from the cache
    trackList.remove(trackList.findTrackById( id ) );

    return 1;
}

int
NjbMediaDevice::deleteItemFromDevice(MediaItem* item, int flags )
{
    DEBUG_BLOCK
    int result = 0;
    if ( isCanceled() || !item )
    {
        return -1;
    }
    MediaItem *next = 0;

    switch( item->type() )
    {
        case MediaItem::TRACK:
            if( isCanceled() )
                break;
            if( item )
            {
                deleteTrack( dynamic_cast<NjbMediaItem *> (item) );
                result++;
            }
            break;
        case MediaItem::ALBUM:
        case MediaItem::ARTIST:
            // Recurse through the lists, slashing and burning.

            if( isCanceled() )
                break;

            expandItem( dynamic_cast<QListViewItem *>(item) );

            for( MediaItem *it = dynamic_cast<MediaItem *>( item->firstChild() ); it ; it = next )
            {

                next = dynamic_cast<MediaItem *>(it->nextSibling());
                int res = deleteItemFromDevice( it, flags );
                if( res >= 0 && result >= 0 )
                    result += res;
                else
                    result = -1;

            }
            if( item )
                delete dynamic_cast<MediaItem *>( item );
            break;
        default:
            result = 0;
    }

    return result;
}

int
NjbMediaDevice::deleteTrack(NjbMediaItem *trackItem)
{
    int status = NJB_Delete_Track( m_njb, trackItem->track()->id() );

    if( status != NJB_SUCCESS)
    {
        debug() << ": NJB_Delete_Track failed" << endl;
        Amarok::StatusBar::instance()->shortLongMessage( i18n( "Deleting failed" ), i18n( "Deleting track(s) failed." ), KDE::StatusBar::Error );
        return -1;
    }

    debug() << ": NJB_Delete_Track track deleted" << endl;

    // remove from the cache
    trackList.remove(trackList.findTrackById( trackItem->track()->id() ) );
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

    for( MediaItem *it = items.first(); it && !(m_canceled); it = items.next() )
    {
        path = destDir.path();
        if( it->type() == MediaItem::TRACK )
        {
            dynamic_cast<MediaBrowser *>( parent() )->queue()->addURL(path, dynamic_cast<MediaItem *>(it) );

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

    KTempDir tempdir( QString::null ); // Default prefix is fine with us
    tempdir.setAutoDelete( true ); // We don't need it once the work is done.
    QString path = tempdir.name(), filepath;
    KURL::List urls;
    for( MediaItem *it = items.first(); it && !(m_canceled); it = items.next() )
    {
        if( (it->type() == MediaItem::TRACK) )
        {
            NjbMediaItem* auxItem = dynamic_cast<NjbMediaItem *>( (it) );
	    if (!auxItem) {
		debug() << "Dynamic cast to NJB media item failed. " << endl;
		return -1;
	    }
            QString track_id;
            track_id.setNum( auxItem->track()->id() );
            filepath = path + auxItem->bundle()->url().path();

            if( NJB_Get_Track( m_njb, auxItem->track()->id(), auxItem->bundle()->filesize(), filepath.utf8(), progressCallback, this)
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
            urls << filepath;
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

    if( m_canceled )
            return 0;
    trackValueList::const_iterator it_track = trackList.findTrackByName( bundle.filename() );
    if( it_track != trackList.end() )
    {
        deleteFromDevice( (*it_track)->id() );
    }

    // read the mp3 header
    int duration = bundle.length();

    if( !duration )
    {
        m_errMsg = i18n( "Not a valid mp3 file");
        return 0;
    }
    MetaBundle temp( bundle );

    NjbTrack *taggedTrack = new NjbTrack();

    taggedTrack->setBundle( temp );

    u_int32_t id;
    m_progressStart = time( 0);
    m_progressMessage = i18n("Copying / Sent %1%...");

    njb_songid_t* songid = NJB_Songid_New();
    taggedTrack->writeToSongid( songid );
    m_busy = true;
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
    trackList.append( taggedTrack );;

    return addTrackToView( taggedTrack );
}

void
NjbMediaDevice::copyTrackFromDevice( MediaItem *item )
{
    DEBUG_BLOCK
    trackValueList::iterator it;
    for( it = trackList.begin(); it != trackList.end(); it++)
        if( (*(*it)->bundle()) == *(item->bundle()) )
            break;

    NjbTrack *track((*it));

    QString filename = item->bundle()->directory() + track->bundle()->filename();
    if( NJB_Get_Track( m_njb, track->id(), track->bundle()->filesize(), filename.utf8(), progressCallback, this)
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
    m_canceled = true;
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
{
    MediaItem *artist = dynamic_cast<MediaItem *>( m_view->findItem( bundle.artist(), 0 ) );
    if ( artist )
    {
        MediaItem *album = dynamic_cast<MediaItem *>( artist->findItem( bundle.album() ) );
        if( album )
        {
            return dynamic_cast<MediaItem *>( album->findItem( bundle.title() ) );
        }
    }
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
        menu.insertItem( SmallIconSet( Amarok::icon( "collection" ) ), i18n("Download file"), DOWNLOAD );
        menu.insertItem( SmallIconSet( Amarok::icon( "collection" ) ), i18n("Download to collection"), DOWNLOAD_TO_COLLECTION );
        menu.insertSeparator();
        //menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), i18n( "Rename" ), RENAME );
        menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "Delete from device" ), DELETE );


        int id =  menu.exec( point );
        MediaItem *i;
        QPtrList<MediaItem> items;
        switch( id )
        {
        case DOWNLOAD:
            downloadSelectedItems();

            break;

        case RENAME:
            //TODO: Implement rename tracks
            //m_view->rename( item, 0 );
            break;

        case DELETE:
            m_view->getSelectedLeaves( 0, &items );
            while( !items.isEmpty() )
            {
                i = items.first();
                MediaDevice::deleteFromDevice( i );
                items.remove(i);
            }
            readJukeboxMusic();
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

    if( njb_media->isCanceled() )
    {
        debug() << "Canceling transfer operation" << endl;
        njb_media->setCanceled( false );
        njb_media->setProgress( sent, total );
        return 1;
    }

    njb_media->setProgress( sent, total);

    return 0;
}

void
NjbMediaDevice::clearItems()
{
    m_view->clear();
}

/** Transfer musical info from the njb to local structures */
int
NjbMediaDevice::readJukeboxMusic( void )
{
//    DEBUG_BLOCK
    int result = NJB_SUCCESS;

    // First, read jukebox tracks
    if(trackList.isEmpty())
    {

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

        for( trackValueList::iterator it = trackList.begin(); it != trackList.end(); it++ )
        {
            if( m_view->findItem( ((*it)->bundle()->artist().string()), 0 ) == 0 )
            {
                NjbMediaItem *artist = new NjbMediaItem( m_view );
                artist->setText( 0, (*it)->bundle()->artist() );
                artist->setType( MediaItem::ARTIST );
                artist->setExpandable( true );
                artist->setBundle( (*it)->bundle() );
                artist->m_device = this;
            }
        }
    }
    debug() << ": return " << result << endl;
    return result;
}

NjbMediaItem *
NjbMediaDevice::addTrackToView( NjbTrack *track, NjbMediaItem *item )
{
    QString artistName = track->bundle()->artist();

    NjbMediaItem *artist = dynamic_cast<NjbMediaItem *>( m_view->findItem( artistName, 0 ) );
    if(!artist)
    {
        artist = new NjbMediaItem(m_view);
        artist->m_device = this;
        artist->setText( 0, artistName );
        artist->setType( MediaItem::ARTIST );
    }

    QString albumName = track->bundle()->album();
    NjbMediaItem *album = dynamic_cast<NjbMediaItem *>( artist->findItem( albumName ) );
    if(!album)
    {
        album = new NjbMediaItem( artist );
        album->setText( 0, albumName );
        album->setType( MediaItem::ALBUM );
        album->m_device = this;
    }

    if( item )
        album->insertItem( item );
    else
    {
        item = new NjbMediaItem( album );
        item->m_device = this;
        QString titleName = track->bundle()->title();
        item->setTrack( track );

        item->setText( 0, titleName );
        item->setType( MediaItem::TRACK );
        item->setBundle( track->bundle() );
        item->track()->setId( track->id() );
    }
    return item;

}

njb_t *
NjbMediaDevice::theNjb()
{
    return NjbMediaDevice::m_njb;
}

void
NjbMediaDevice::expandItem( QListViewItem *item )
{
    DEBUG_BLOCK
    // First clear the item's children to repopulate.
    while( item->firstChild() )
        delete item->firstChild();

    NjbMediaItem *it = dynamic_cast<NjbMediaItem *>( item );

    if (!it) {
	debug() << "Dynamic cast to NJB media item failed" << endl;
	return;
    }

    switch( it->type() )
    {
        case MediaItem::ARTIST:
            if( it->childCount() == 0 ) // Just to be sure
                addAlbums( item->text( 0 ), it );
            break;
        case MediaItem::ALBUM:
            if( it->childCount() == 0 )
                addTracks( it->bundle()->artist(), item->text( 0 ), it );
            break;
        default:
            break;
    }
}

NjbMediaItem*
NjbMediaDevice::addAlbums(const QString &artist, NjbMediaItem *item)
{
    for( trackValueList::iterator it = trackList.begin(); it != trackList.end(); it++ )
    {
        if( item->findItem( (*it)->bundle()->album() ) == 0 && ( (*it)->bundle()->artist().string() == artist ) )
        {
            NjbMediaItem *album = new NjbMediaItem( item );
            album->setText( 0, (*it)->bundle()->album() );
            album->setType( MediaItem::ALBUM );
            album->setExpandable( true );
            album->setBundle( (*it)->bundle() );
            album->m_device = this;
        }
    }
    return item;
}

NjbMediaItem*
NjbMediaDevice::addTracks(const QString &artist, const QString &album, NjbMediaItem *item)
{
    for( trackValueList::iterator it = trackList.begin(); it != trackList.end(); it++ )
    {
        if( ( (*it)->bundle()->album().string() == album ) && ( (*it)->bundle()->artist().string() == artist ))
        {
            NjbMediaItem *track = new NjbMediaItem( item );
            track->setText( 0, (*it)->bundle()->title() );
            track->setType( MediaItem::TRACK );
            track->setBundle( (*it)->bundle() );
            track->setTrack( (*it) );
            track->m_device = this;
        }
    }
    return item;
}

NjbMediaItem*
NjbMediaDevice::addArtist( NjbTrack *track )
{
    if( m_view->findItem( track->bundle()->artist().string(), 0 ) == 0 )
    {
        NjbMediaItem *artist = new NjbMediaItem( m_view );
        artist->setText( 0, track->bundle()->artist() );
        artist->setType( MediaItem::ARTIST );
        artist->setExpandable( true );
        artist->setBundle( track->bundle() );
        artist->m_device = this;
    }
    return dynamic_cast<NjbMediaItem *>( m_view->findItem( track->bundle()->artist().string(), 0 ) );
}

void
NjbMediaDevice::customClicked()
{
    QString Information;
    QString tracksFound;
    QString powerStatus;
    QString batteryLevel;
    QString batteryCharging;

    if( m_connected )
    {
        tracksFound = i18n( "1 track found on device",
                            "%n tracks found on device ", trackList.size() );
        powerStatus = ( (NJB_Get_Auxpower( m_njb ) == 1) ? i18n("On auxiliary power") : i18n("On main power") );
        batteryCharging = ( (NJB_Get_Battery_Charging( m_njb ) == 1) ? i18n("Battery charging") : i18n("Battery not charging") );
        batteryLevel = (i18n("Battery level: ") + QString::number( NJB_Get_Battery_Level( m_njb ) ) );

        Information = ( i18n("Player Information for ") + m_name +'\n' +
                        i18n("Power status: ") + powerStatus + '\n' +
                        i18n("Battery status: ") + batteryLevel + " (" +
                        batteryCharging + ')' );
    }
    else
    {
        Information = i18n("Player not connected");
    }

    KMessageBox::information(0, Information, i18n("Device information") );
}
