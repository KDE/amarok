/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "UpnpCollectionFactory"
#include "UpnpCollectionFactory.h"

#include <kio/jobclasses.h>
#include <kio/scheduler.h>
#include <kio/netaccess.h>
#include <kdirlister.h>
#include <kurl.h>

#include "core/support/Debug.h"
#include "UpnpCollection.h"

namespace Collections {

AMAROK_EXPORT_COLLECTION( UpnpCollectionFactory, upnpcollection )

UpnpCollectionFactory::UpnpCollectionFactory( QObject *parent, const QVariantList &args )
    : Collections::CollectionFactory()
    , m_networkLister(0)
{
    setParent( parent );
    Q_UNUSED( args );
}

UpnpCollectionFactory::~UpnpCollectionFactory()
{
    delete m_networkLister;
}

// TODO need to monitor upnp devices somehow

void UpnpCollectionFactory::init()
{
    DEBUG_BLOCK

    m_networkLister = new KDirLister( this );
    connect(m_networkLister, SIGNAL(newItems(const KFileItemList &)),
            this, SLOT(slotNewDevices(const KFileItemList &)) );
    connect(m_networkLister, SIGNAL(itemsDeleted(const KFileItemList &)),
            this, SLOT(slotDeviceOffline(const KFileItemList &)) );

    m_networkLister->openUrl(KUrl("network:/"));
    emit newCollection( new UpnpCollection( "upnp-ms://bf7eace9-e63f-4267-a871-7b572d750653", "MediaTomb" ) );

}

void UpnpCollectionFactory::slotNewDevices( const KFileItemList &list )
{
    foreach( KFileItem item, list ) {
// TODO only add certain devices
        m_networkLister->openUrl(item.url(), KDirLister::Keep);
        if( item.isReadable() )
            createCollection( item );
    }
}

void UpnpCollectionFactory::createCollection( const KFileItem &item )
{
//    QRegExp mediaServerExp("inode/vnd.kde.service.upnp.MediaServer[123]");
//    if( mediaServerExp.exactMatch(item.mimetype()) )
//        emit newCollection( new UpnpCollection( item.url().prettyUrl(), item.text() ) );
}

void UpnpCollectionFactory::slotDeviceOffline( const KFileItemList &list )
{
// TODO have a map of all collections and use that to tell
// collection to remove itself since we can't have
// each collection monitoring the network kioslave
}

}
