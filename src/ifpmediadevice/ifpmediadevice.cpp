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

 /**
  *  iRiver ifp media device code
  *  @see http://ifp-driver.sourceforge.net/libifp/docs/ifp_8h.html
  *  @note ifp uses a backslash '\' as a directory delimiter for _remote_ files
  */

#define DEBUG_PREFIX "IfpMediaDevice"

#include "ifpmediadevice.h"

#include "debug.h"
#include "metabundle.h"
#include "collectiondb.h"

#include <kiconloader.h>       //smallIcon
#include <kmessagebox.h>

#include <qfile.h>
#include <qcstring.h>

/**
 * IfpMediaItem Class
 */

class IfpMediaItem : public MediaItem
{
    public:
        IfpMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        IfpMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        void
        IfpMediaItem::setEncodedName( QString &name )
        {
            m_encodedName = QFile::encodeName( name );
        }

        void
        IfpMediaItem::setEncodedName( QCString &name ) { m_encodedName = name; }

        QCString
        IfpMediaItem::encodedName() { return m_encodedName; }

        // List directories first, always
        int
        IfpMediaItem::compare( QListViewItem *i, int col, bool ascending ) const
        {
            #define i static_cast<IfpMediaItem *>(i)
            switch( type() )
            {
                case MediaItem::DIRECTORY:
                    if( i->type() == MediaItem::DIRECTORY )
                        break;
                    return -1;

                default:
                    if( i->type() == MediaItem::DIRECTORY )
                        return 1;
            }
            #undef i

            return MediaItem::compare(i, col, ascending);
        }

    private:
        bool     m_dir;
        QCString m_encodedName;
};


/**
 * IfpMediaDevice Class
 */

IfpMediaDevice::IfpMediaDevice( MediaDeviceView* parent, MediaDeviceList *listview )
    : MediaDevice( parent, listview )
    , m_dev( 0 )
    , m_dh( 0 )
    , m_connected( false )
    , m_tmpParent( 0 )
{
    DEBUG_BLOCK
}

IfpMediaDevice::~IfpMediaDevice()
{
    DEBUG_BLOCK

    closeDevice();
}

bool
IfpMediaDevice::checkResult(int result, QString message)
{
    if( result == 0 )
        return true;

    error() << result << ": " << message << endl;
    return false;
}


bool
IfpMediaDevice::openDevice( bool /*silent*/ )
{
    DEBUG_BLOCK

    usb_init();

    m_dh = (usb_dev_handle*)ifp_find_device();


    if( m_dh == NULL )
    {
        error() << "A suitable iRiver iFP device couldn't be found" << endl;
        return false;
    }

    m_dev = usb_device( m_dh );
    if( m_dev == NULL )
    {
        error() << "Could not get usb_device()" << endl;
        if( ifp_release_device( m_dh ) )
            error() << "warning: release_device failed." << endl;
        return false;
    }

    /* "must be called" written in the libusb documentation */
    if( usb_claim_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber ) )
    {
        error() << "Device is busy.  (I was unable to claim its interface.)" << endl;

        if( ifp_release_device( m_dh ) )
            error() << "warning: release_device failed." << endl;
        return false;
    }

    int i = ifp_init( &m_ifpdev, m_dh );
    if( i )
    {
        error() << "IFP device: Device cannot be opened." << endl;

        usb_release_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber );
        return false;
    }

    m_connected = true;

    listDir( "" );

    return true;
}

bool
IfpMediaDevice::closeDevice()  //SLOT
{
    DEBUG_BLOCK

    if( m_connected )
    {

        if( m_dh )
        {
            usb_release_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber );

            if( ifp_release_device( m_dh ) )
                error() << "warning: release_device failed." << endl;

            ifp_finalize( &m_ifpdev );
            m_dh = 0;
        }

        m_listview->clear();

        m_connected = false;
    }

    return true;
}

void
IfpMediaDevice::renameItem( QListViewItem *item ) // SLOT
{
    if( !item )
        return;

    #define item static_cast<IfpMediaItem*>(item)

    QCString src  = QFile::encodeName( getFullPath( item, false ) );
    src.append( item->encodedName() );

    QCString dest = "\\" + QFile::encodeName( item->text(0) );

    debug() << "Renaming " << src << " to: " << dest << endl;

    if( ifp_rename( &m_ifpdev, src, dest ) ) //success == 0
        //rename failed
        item->setText( 0, item->encodedName() );

    #undef item
}


void
IfpMediaDevice::expandItem( QListViewItem *item ) // SLOT
{
    if( !item || !item->isExpandable() ) return;

    while( item->firstChild() )
        delete item->firstChild();

    m_tmpParent = item;

    QString path = getFullPath( item );
    listDir( path );

    m_tmpParent = 0;
}


MediaItem *
IfpMediaDevice::copyTrackToDevice( const MetaBundle& bundle, bool /*isPodcast*/ )
{
    const KURL &url = bundle.url();

    const QCString src = QFile::encodeName( url.path() );
    const QCString dest = QFile::encodeName( "\\" + url.filename() ); // TODO: add to directory

    int result = uploadTrack( src, dest );

    checkResult( result, i18n("Could not upload: %1").arg(dest) );

    if( !result ) //success
        addTrackToList( IFP_FILE, url.filename() );

    return m_last;
}

int
IfpMediaDevice::uploadTrack( const QCString& src, const QCString& dest )
{
    debug() << "Transferring " << src << " to: " << dest << endl;

    return ifp_upload_file( &m_ifpdev, src, dest, 0, 0 /*uploadCallback, this */);
}

int
IfpMediaDevice::uploadCallback( void */*pData*/, ifp_transfer_status */*progress*/ )
{
    return 0;
    // will be called by 'ifp_upload_file_with_callback'
//     return static_cast<IfpMediaDevice *>(pData)->uploadCallback( buf, size );
}


/// Deleting

bool
IfpMediaDevice::deleteItemFromDevice( MediaItem *item, bool /*onlyPlayed*/ )
{
    if( !item )
        return false;

    QString path = getFullPath( item );

    QCString encodedPath = QFile::encodeName( path );
    debug() << "Deleting file: " << encodedPath << endl;
    int err;

    switch( item->type() )
    {
        case MediaItem::DIRECTORY:
            err = ifp_rmdir( &m_ifpdev, encodedPath );
            checkResult( err, i18n("Directory does not exist: '%1'").arg(encodedPath) );
            break;

        default:
            err = ifp_delete( &m_ifpdev, encodedPath );
            checkResult( err, i18n("File does not exist: '%1'").arg(encodedPath) );
            break;
    }
    if( err == 0 ) //success
        delete item;

    return (err == 0);
}

/// Directory Reading

void
IfpMediaDevice::listDir( const QString &dir )
{
    DEBUG_BLOCK

    debug() << "listing contents in: '" << dir << "'" << endl;

    int err = ifp_list_dirs( &m_ifpdev, dir.ascii(), listDirCallback, this );
    checkResult( err, i18n("Cannot enter directory: '%1'").arg(dir) );
}

// will be called by 'ifp_list_dirs'
int
IfpMediaDevice::listDirCallback( void *pData, int type, const char *name, int size )
{
    QString qName( name );
    return static_cast<IfpMediaDevice *>(pData)->addTrackToList( type, qName, size );
}

int
IfpMediaDevice::addTrackToList( int type, QString name, int /*size*/ )
{
    m_tmpParent ?
            m_last = new IfpMediaItem( m_tmpParent ):
            m_last = new IfpMediaItem( m_listview );

    if( type == IFP_DIR ) //directory
        m_last->setType( MediaItem::DIRECTORY );

    else if( type == IFP_FILE ) //file
    {
        if( name.endsWith( "mp3", false ) || name.endsWith( "wma", false ) ||
            name.endsWith( "wav", false ) || name.endsWith( "ogg", false ) ||
            name.endsWith( "asf", false ) )

            m_last->setType( MediaItem::TRACK );

        else
            m_last->setType( MediaItem::UNKNOWN );
    }
    m_last->setEncodedName( name );
    m_last->setText( 0, name );
    return 0;
}

/// Capacity, in kB

bool
IfpMediaDevice::getCapacity( unsigned long *total, unsigned long *available )
{
    return false;
//     int totalBytes = ifp_capacity( &m_ifpdev );
//     int freeBytes = ifp_capacity( &m_ifpdev );
//
//     unsigned long kb;
//
//     *total = (unsigned long)totalBytes / kb;
//     *available = (unsigned long)freeBytes / kb;
//
//     return totalBytes > 0;
}

QString
IfpMediaDevice::getFullPath( const QListViewItem *item, const bool getFilename )
{
    if( !item ) return QString::null;

    QString path;

    if( getFilename ) path = item->text(0);

    QListViewItem *parent = item->parent();
    while( parent )
    {
        path.prepend( "\\" );
        path.prepend( parent->text(0) );
        parent = parent->parent();
    }
    path.prepend( "\\" );

    return path;
}


#include "ifpmediadevice.moc"
