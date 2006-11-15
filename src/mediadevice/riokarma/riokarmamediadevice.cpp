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
  *  Rio Karma media device
  *  @author Andy Kelk <andy@mopoke.co.uk>
  *  @see http://linux-karma.sourceforge.net/
  */

#define DEBUG_PREFIX "RioKarmaMediaDevice"

#include <config.h>
#include "riokarmamediadevice.h"

AMAROK_EXPORT_PLUGIN( RioKarmaMediaDevice )

// Amarok
#include <debug.h>
#include <metabundle.h>
#include <statusbar/statusbar.h>
#include <statusbar/popupMessage.h>

// KDE
#include <kapplication.h>
#include <kiconloader.h>
#include <kpopupmenu.h>

// Qt
#include <qdir.h>
#include <qlistview.h>
#include <qmap.h>


/**
 * RioKarmaMediaDevice Class
 */

RioKarmaMediaDevice::RioKarmaMediaDevice() : MediaDevice()
{
    m_name = "Rio Karma";
    setDisconnected();
    m_hasMountPoint = true;
    m_syncStats = false;
    m_transcode = false;
    m_transcodeAlways = false;
    m_transcodeRemove = false;
    m_configure = false;
    m_customButton = false;
    m_transfer = true;
}

void
RioKarmaMediaDevice::init( MediaBrowser *parent )
{
    MediaDevice::init( parent );
}

bool
RioKarmaMediaDevice::isConnected()
{
    return m_rio >= 0 ? true : false;
}

/**
 * File types that we support
 */
QStringList
RioKarmaMediaDevice::supportedFiletypes()
{
    QStringList supportedFiles;
    supportedFiles << "mp3";
    supportedFiles << "ogg";
    supportedFiles << "wma";
    supportedFiles << "flac";
    return supportedFiles;
}

/**
 * Copy a track to the device
 */
MediaItem
*RioKarmaMediaDevice::copyTrackToDevice( const MetaBundle &bundle )
{
    DEBUG_BLOCK

    QString genericError = i18n( "Could not send track" );

    if( m_fileNameToItem[ bundle.filename() ] != 0 )
    {
        // track already exists. don't do anything (for now).
        debug() << "Track already exists on device." << endl;
        Amarok::StatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "Track already exists on device" ),
            KDE::StatusBar::Error
        );
        return 0;
    }

    int fid = lk_rio_write( m_rio, bundle.url().path().utf8() );

    if( fid < 0 )
    {
        debug() << "Could not write file" << fid << endl;
        return 0;
    }

    MetaBundle temp( bundle );
    RioKarmaTrack *taggedTrack = new RioKarmaTrack( fid );
    taggedTrack->setBundle( temp );

    // cache the track
    updateRootItems();
    return addTrackToView( taggedTrack );
}

/**
 * Write any pending changes to the device, such as database changes
 */
void
RioKarmaMediaDevice::synchronizeDevice()
{
    DEBUG_BLOCK
    int ret;
    ret = lk_karma_write_smalldb();
    if( ret )
        debug() << "error writing smalldb file" << endl;
}

/**
 * Find an existing track
 */
MediaItem
*RioKarmaMediaDevice::trackExists( const MetaBundle &bundle )
{
    MediaItem *artist = dynamic_cast<MediaItem *>( m_view->findItem( bundle.artist(), 0 ) );
    if( artist )
    {
        MediaItem *album = dynamic_cast<MediaItem *>( artist->findItem( bundle.album() ) );
        if( album )
        {
            MediaItem *track =  dynamic_cast<MediaItem *>( album->findItem( bundle.title() ) );
            if( track )
            {
                if( track->bundle()->track() == bundle.track() )
                    return track;
            }
        }
    }
    return 0;
}

/**
 * Create a new playlist
 * @note Playlists not implemented yet... :-)
 */
RioKarmaMediaItem
*RioKarmaMediaDevice::newPlaylist( const QString &name, MediaItem *parent, QPtrList<MediaItem> items )
{
    Q_UNUSED( name );
    Q_UNUSED( parent );
    Q_UNUSED( items );
    return 0;
}

/**
 * Add an item to a playlist
 * @note Playlists not implemented yet... :-)
 */
void
RioKarmaMediaDevice::addToPlaylist( MediaItem *mlist, MediaItem *after, QPtrList<MediaItem> items )
{
    Q_UNUSED( mlist );
    Q_UNUSED( after );
    Q_UNUSED( items );
}

/**
 * Recursively remove MediaItem from the device
 */
int
RioKarmaMediaDevice::deleteItemFromDevice(MediaItem* item, int flags )
{

    int result = 0;
    if( isCanceled() )
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
                int res = deleteRioTrack( dynamic_cast<RioKarmaMediaItem *> ( item ) );
                if( res >=0 && result >= 0 )
                    result += res;
                else
                    result = -1;
            }
            break;
        case MediaItem::ALBUM:
        case MediaItem::ARTIST:
            // Recurse through the lists
            next = 0;

            if( isCanceled() )
                break;

            for( MediaItem *it = dynamic_cast<MediaItem *>( item->firstChild() ); it ; it = next )
            {
                next = dynamic_cast<MediaItem *>( it->nextSibling() );
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

/**
 * Actually delete a track from the Rio
 */
int
RioKarmaMediaDevice::deleteRioTrack( RioKarmaMediaItem *trackItem )
{

    DEBUG_BLOCK

    debug() << "delete this fid : " << trackItem->track()->id() << endl;

    // delete the file
    int status = lk_karma_delete_file( m_rio, trackItem->track()->id() );
    if( status < 0 ) {
        debug() << "delete track failed" << endl;
        return -1;
    }
    debug() << "track deleted" << endl;

    // delete the properties (db entry)
    status = lk_properties_del_property( trackItem->track()->id() );
    if( status < 0 ) {
        debug() << "delete property failed" << endl;
        return -1;
    }
    debug() << "property deleted" << endl;

    // remove from the listview
    delete trackItem;
    kapp->processEvents( 100 );

    return 1;
}

/**
 * Connect to device, and populate m_view with MediaItems
 */
bool
RioKarmaMediaDevice::openDevice( bool silent )
{
    DEBUG_BLOCK

    Q_UNUSED( silent );

    QDir dir( mountPoint() );
    if( !dir.exists() )
    {
        Amarok::StatusBar::instance()->longMessage(
                i18n( "Media device: Mount point %1 does not exist" ).arg( mountPoint() ),
                KDE::StatusBar::Error );
        return false;
    }

    if( m_rio >= 0 )
        return true;

    QString genericError = i18n( "Could not connect to Rio Karma" );

    char *mount = qstrdup( mountPoint().utf8() );
    m_rio = lk_karma_connect( mount );

    debug() << "Rio karma : " << m_rio << endl;

    if( m_rio < 0 )
    {
        debug()<< "Error connecting" << endl;
        Amarok::StatusBar::instance()->shortLongMessage( genericError, i18n( "Rio Karma could not be opened" ), KDE::StatusBar::Error );
        setDisconnected();
        return false;
    }

    lk_karma_use_smalldb();

    lk_karma_write_dupes( 1 );

    RioKarmaMediaDevice::readKarmaMusic();

    return true;
}

/**
 * Wrap up any loose ends and close the device
 */
bool
RioKarmaMediaDevice::closeDevice()
{
    DEBUG_BLOCK
    clearItems();
    setDisconnected();
    return true;
}

/**
 * Get the capacity and freespace available on the device, in KB
 */
bool
RioKarmaMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if( !isConnected() )
        return false;

    uint32_t numfiles;
    uint64_t disksize;
    uint64_t freespace;
    uint32_t maxfileid;

    if( lk_karma_get_storage_details( m_rio, 0, &numfiles, &disksize, &freespace, &maxfileid ) == 0 )
    {
        *total = disksize;
        *available = freespace;

        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Current device ID (usually starts at 0)
 */
int
RioKarmaMediaDevice::current_id()
{
    return m_rio;
}

/**
 * We use -1 device ID to show a disconnected device.
 * This just sets the device ID to that.
 */
void
RioKarmaMediaDevice::setDisconnected()
{
    m_rio = -1;
}

/**
 * Handle clicking of the right mouse button
 */
void
RioKarmaMediaDevice::rmbPressed( QListViewItem *qitem, const QPoint &point, int )
{

    enum Actions {DELETE};

    RioKarmaMediaItem *item = static_cast<RioKarmaMediaItem *>( qitem );
    if( item )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "Delete from device" ), DELETE );

        int id =  menu.exec( point );
        switch( id )
        {
        case DELETE:
            MediaDevice::deleteFromDevice();
            break;
        }
        return;
    }

}

/**
 * Add a track to the current list view
 */
RioKarmaMediaItem
*RioKarmaMediaDevice::addTrackToView( RioKarmaTrack *track, RioKarmaMediaItem *item )
{
    QString artistName = track->bundle()->artist();

    RioKarmaMediaItem *artist = dynamic_cast<RioKarmaMediaItem *>( m_view->findItem( artistName, 0 ) );
    if( !artist )
    {
        artist = new RioKarmaMediaItem( m_view );
        artist->m_device = this;
        artist->setText( 0, artistName );
        artist->setType( MediaItem::ARTIST );
    }

    QString albumName = track->bundle()->album();
    RioKarmaMediaItem *album = dynamic_cast<RioKarmaMediaItem *>( artist->findItem( albumName ) );
    if( !album )
    {
        album = new RioKarmaMediaItem( artist );
        album->setText( 0, albumName );
        album->setType( MediaItem::ALBUM );
        album->m_device = this;
    }

    if( item )
        album->insertItem( item );
    else
    {
        item = new RioKarmaMediaItem( album );
        item->m_device = this;
        QString titleName = track->bundle()->title();
        item->setTrack( track );
        item->m_order = track->bundle()->track();
        item->setText( 0, titleName );
        item->setType( MediaItem::TRACK );
        item->setBundle( track->bundle() );
        item->track()->setId( track->id() );
        m_fileNameToItem[ track->bundle()->filename() ] = item;
    }
    return item;
}

/**
 * Get karma tracks and add them to the listview
 */
int
RioKarmaMediaDevice::readKarmaMusic()
{

    DEBUG_BLOCK

    clearItems();


    QString genericError = i18n( "Could not get music from Rio Karma" );

    int total = 100;
    int progress = 0;
    setProgress( progress, total ); // we don't know how many tracks. fake progress bar.

    kapp->processEvents( 100 );

    lk_karma_load_database( m_rio );

    int i;
    uint32_t *ret;

    kapp->processEvents( 100 );

    ret = lk_properties_andOrSearch( 0, 0, "fid", "" );

    if( ret == 0 )
    {
        debug()<< "Error reading tracks. NULL returned." << endl;
        Amarok::StatusBar::instance()->shortLongMessage( genericError, i18n( "Could not read Rio Karma tracks" ), KDE::StatusBar::Error );
        setDisconnected();
        hideProgress();
        return -1;
    }

    total = 0;
    // spin through once to determine size of the list
    for( i=0; ret[i] != 0; i++ )
    {
        total++;
    }
    setProgress( progress, total );
    // now process the tracks
    for( i=0; ret[i] != 0; i++ )
    {
        // check playlist
        if( qstrcmp( "playlist", lk_properties_get_property( ret[i], "type" ) ) == 0 )
        {
            // nothing for now...
            debug() << "Found a playlist at fid " << ret[i] << ". Skipping." << endl;
        }
        else
        {
            RioKarmaTrack *track = new RioKarmaTrack( ret[i] );
            track->readMetaData();
            addTrackToView( track );
        }
        progress++;
        setProgress( progress );
        if( progress % 50 == 0 )
            kapp->processEvents( 100 );
    }
    setProgress( total );
    hideProgress();

    return 0;
}

/**
 * Clear the current listview
 */
void
RioKarmaMediaDevice::clearItems()
{
    m_view->clear();
}

/**
 * RioKarmaTrack Class
 */
RioKarmaTrack::RioKarmaTrack( int Fid )
{
    m_id = Fid;
}


RioKarmaTrack::~RioKarmaTrack()
{
    m_itemList.setAutoDelete( true );
    while( m_itemList.count() > 0 )
    {
        delete m_itemList.first();
    }
}

/**
 * Read track properties from the Karma and set it on the track
 */
void
RioKarmaTrack::readMetaData()
{
    MetaBundle *bundle = new MetaBundle();

    bundle->setGenre( AtomicString( QString::fromUtf8( lk_properties_get_property( m_id, "genre" ) ) ) );
    bundle->setArtist( AtomicString( QString::fromUtf8( lk_properties_get_property( m_id, "artist" ) ) ) );
    bundle->setAlbum( AtomicString( QString::fromUtf8( lk_properties_get_property( m_id, "source" ) ) ) );
    bundle->setTitle( AtomicString( QString::fromUtf8( lk_properties_get_property( m_id, "title" ) ) ) );

    // translate codecs to file types
    QString codec = QCString( lk_properties_get_property( m_id, "codec" ) );
    if( codec == "mp3" )
        bundle->setFileType( MetaBundle::mp3 );
    else if( codec == "wma" )
        bundle->setFileType( MetaBundle::wma );
    else if( codec == "flac" )
        bundle->setFileType( MetaBundle::flac );
    else if( codec == "vorbis" )
        bundle->setFileType( MetaBundle::ogg );
    else
        bundle->setFileType( MetaBundle::other );

    bundle->setYear( QString( lk_properties_get_property( m_id, "year" ) ).toUInt() );
    bundle->setTrack( QString( lk_properties_get_property( m_id, "tracknr" ) ).toUInt() );
    bundle->setLength( QString( lk_properties_get_property( m_id, "duration" ) ).toUInt() );

    this->setBundle( *bundle );
}

/**
 * Set this track's metabundle
 */
void
RioKarmaTrack::setBundle( MetaBundle &bundle )
{
    m_bundle = bundle;
}

/**
 * Add a child item
 */
void
RioKarmaTrack::addItem( const RioKarmaMediaItem *item )
{
    m_itemList.append( item );
}

/**
 * Remove a child item
 */
bool
RioKarmaTrack::removeItem( const RioKarmaMediaItem *item )
{
    m_itemList.remove( item );
    return m_itemList.isEmpty();
}
