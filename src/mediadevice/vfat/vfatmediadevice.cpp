/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 *                                                                         *
 * With some code helpers from KIO_VFAT                                     *
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


#define DEBUG_PREFIX "VfatMediaDevice"

#include "vfatmediadevice.h"

AMAROK_EXPORT_PLUGIN( VfatMediaDevice )

#include "debug.h"
#include "medium.h"
#include "metabundle.h"
#include "collectiondb.h"
#include "statusbar/statusbar.h"

#include <kapplication.h>
#include <kconfig.h>           //download saveLocation
#include <kiconloader.h>       //smallIcon
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kurlrequester.h>     //downloadSelectedItems()
#include <kurlrequesterdlg.h>  //downloadSelectedItems()

#include <qfile.h>
#include <qcstring.h>

namespace amaroK { extern KConfig *config( const QString& ); }

/**
 * VfatMediaItem Class
 */

class VfatMediaItem : public MediaItem
{
    public:
        VfatMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        VfatMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        void
        setEncodedName( QString &name )
        {
            m_encodedName = QFile::encodeName( name );
        }

        void
        setEncodedName( QCString &name ) { m_encodedName = name; }

        QCString
        encodedName() { return m_encodedName; }

        // List directories first, always
        int
        compare( QListViewItem *i, int col, bool ascending ) const
        {
            #define i static_cast<VfatMediaItem *>(i)
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
 * VfatMediaDevice Class
 */

VfatMediaDevice::VfatMediaDevice()
    : MediaDevice()
    , m_connected( false )
    , m_tmpParent( 0 )
{
    m_name = "VFAT Device";
}

void
VfatMediaDevice::init( MediaBrowser* parent )
{
    MediaDevice::init( parent );
}

VfatMediaDevice::~VfatMediaDevice()
{
    closeDevice();
}

bool
VfatMediaDevice::checkResult( int result, QString message )
{
    if( result == 0 )
        return true;

    error() << result << ": " << message << endl;
    return false;
}


bool
VfatMediaDevice::openDevice( bool /*silent*/ )
{
    DEBUG_BLOCK

    m_connected = true;

    listDir( "" );

    return true;
}

bool
VfatMediaDevice::closeDevice()  //SLOT
{
    DEBUG_BLOCK

    if( m_connected )
    {
        m_view->clear();

        m_connected = false;
    }

    return true;
}

/// Renaming

void
VfatMediaDevice::renameItem( QListViewItem */*item*/ ) // SLOT
{

    return; //NOT IMPLEMENTED YET

}

/// Creating a directory

MediaItem *
VfatMediaDevice::newDirectory( const QString &name, MediaItem */*parent*/ )
{

    if( !m_connected || name.isEmpty() ) return 0;

    else return m_last; //NOT IMPLEMENTED YET

}

void
VfatMediaDevice::addToDirectory( MediaItem *directory, QPtrList<MediaItem> items )
{
    if( !directory || items.isEmpty() ) return;

    return; //NOT IMPLEMENTED YET
}

/// Uploading

MediaItem *
VfatMediaDevice::copyTrackToDevice( const MetaBundle& /*bundle*/, const PodcastInfo* /*info*/ )
{
    if( !m_connected ) return 0;

    return 0; //NOT IMPLEMENTED YET
}

/// File transfer methods

int
VfatMediaDevice::uploadTrack( const QCString& src, const QCString& dest )
{
    debug() << "Transferring " << src << " to: " << dest << endl;

    return 0;  //NOT IMPLEMENTED YET
}

int
VfatMediaDevice::downloadTrack( const QCString& src, const QCString& dest )
{
    debug() << "Downloading " << src << " to: " << dest << endl;

    return 0;  //NOT IMPLEMENTED YET
}

void
VfatMediaDevice::downloadSelectedItems()
{
    return;  //NOT IMPLEMENTED YET
}

/// Deleting

int
VfatMediaDevice::deleteItemFromDevice( MediaItem *item, bool /*onlyPlayed*/ )
{
    if( !item || !m_connected ) return -1;

    return 0;  //NOT IMPLEMENTED YET
}

/// Directory Reading

void
VfatMediaDevice::expandItem( QListViewItem */*item*/ ) // SLOT
{
    return;  //NOT IMPLEMENTED YET
}

void
VfatMediaDevice::listDir( const QString &/*dir*/ )
{
    return;  //NOT IMPLEMENTED YET
}

int
VfatMediaDevice::listDirCallback( void */*pData*/, int /*type*/, const char */*name*/, int /*size*/ )
{
    return 0;  //NOT IMPLEMENTED YET
}

int
VfatMediaDevice::addTrackToList( int /*type*/, QString /*name*/, int /*size*/ )
{
    return 0; //NOT IMPLEMENTED YET
}

/// Capacity, in kB

bool
VfatMediaDevice::getCapacity( unsigned long */*total*/, unsigned long */*available*/ )
{
    if( !m_connected ) return false;

    return true; //NOT IMPLEMENTED YET
}

/// Helper functions

QString
VfatMediaDevice::getFullPath( const QListViewItem *item, const bool getFilename )
{
    if( !item ) return QString::null;

    QString path;

    if (getFilename) path = item->text(0);

    QListViewItem *parent = item->parent();
    while( parent )
    {
        path.prepend( "/" );
        path.prepend( parent->text(0) );
        parent = parent->parent();
    }
    path.prepend( m_medium->mountPoint() );

    return path;

}


void
VfatMediaDevice::rmbPressed( QListViewItem* /*qitem*/, const QPoint& /*point*/, int )
{
    return; //NOT IMPLEMENTED YET
}

#include "vfatmediadevice.moc"
