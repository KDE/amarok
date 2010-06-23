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

#define DEBUG_PREFIX "UpnpSearchCollection"

#include "UpnpSearchCollection.h"

#include "core/support/Debug.h"
#include "statusbar/StatusBar.h"
#include "UpnpQueryMaker.h"
#include "UpnpMeta.h"

#include <QStringList>
#include <QTimer>

#include <KLocale>
#include <KMessageBox>
#include <kdatetime.h>
#include <kio/upnptypes.h>
#include <kio/scheduler.h>
#include <kio/jobclasses.h>

using namespace Meta;

namespace Collections {

//UpnpSearchCollection

// TODO register for the device bye bye and emit remove()
UpnpSearchCollection::UpnpSearchCollection( const DeviceInfo &info )
    : UpnpCollectionBase( info )
    , m_fullScanInProgress( false )
{
    DEBUG_BLOCK

    // experimental code, will probably be moved to a better place
    OrgKdeKDirNotifyInterface *notify = new OrgKdeKDirNotifyInterface("", "", QDBusConnection::sessionBus(), this );
    Q_ASSERT(connect( notify, SIGNAL( FilesChanged(const QStringList &) ),
                      this, SLOT( slotFilesChanged(const QStringList &) ) ));
}

UpnpSearchCollection::~UpnpSearchCollection()
{
}

void UpnpSearchCollection::slotFilesChanged(const QStringList &list )
{
    if( m_fullScanInProgress )
        return;

    debug() << "Files changed" << list;
}

void
UpnpSearchCollection::startFullScan()
{
    DEBUG_BLOCK;

// TODO change this to "/" when we have files changed being
/// ignored for full scans.
// right now its good to have the full scan finish quickly for
// development purposes
    startIncrementalScan( "/" );
    m_fullScanInProgress = true;
    m_fullScanTimer = new QTimer( this );
    m_fullScanTimer->start(5000);
}

void
UpnpSearchCollection::startIncrementalScan( const QString &directory )
{
    DEBUG_BLOCK;
    if( m_fullScanInProgress ) {
        debug() << "Full scan in progress, aborting";
        return;
    }
    debug() << "Scanning directory" << directory;
    KUrl url;
    url.setScheme( "upnp-ms" );
    url.setHost( m_deviceInfo.uuid() );
    url.setPath( directory );
}

QueryMaker*
UpnpSearchCollection::queryMaker()
{
    DEBUG_BLOCK;
    return new UpnpQueryMaker( this );
}

bool
UpnpSearchCollection::possiblyContainsTrack( const KUrl &url ) const
{
    debug() << "Requested track " << url;
    return false;
}

} //~ namespace
#include "UpnpSearchCollection.moc"

