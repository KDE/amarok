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

#include "UpnpQueryMaker.h"
#include "UpnpMeta.h"
#include "UpnpCache.h"
#include "upnptypes.h"

#include <QStringList>
#include <QTimer>

#include <KLocalizedString>
#include <KMessageBox>
#include <KIO/Scheduler>

using namespace Meta;

namespace Collections {

//UpnpSearchCollection

// TODO register for the device bye bye and emit remove()
UpnpSearchCollection::UpnpSearchCollection( const DeviceInfo& dev, QStringList searchCapabilities )
    : UpnpCollectionBase( dev )
    , m_searchCapabilities( searchCapabilities )
    , m_cache( new UpnpCache( this ) )
{
    DEBUG_BLOCK

    OrgKdeKDirNotifyInterface *notify = new OrgKdeKDirNotifyInterface("", "", QDBusConnection::sessionBus(), this );
    connect( notify, &OrgKdeKDirNotifyInterface::FilesChanged, this, &UpnpSearchCollection::slotFilesChanged );
}

UpnpSearchCollection::~UpnpSearchCollection()
{
}

void UpnpSearchCollection::slotFilesChanged(const QStringList &list )
{
    debug() << "Files changed" << list;
}

QueryMaker*
UpnpSearchCollection::queryMaker()
{
    DEBUG_BLOCK;
    return new UpnpQueryMaker( this );
}

Meta::TrackPtr
UpnpSearchCollection::trackForUrl( const QUrl &url )
{
#ifdef __GNUC__
    #warning Implement track for url
#endif
    // TODO FIXME how to do this?
    debug() << "Requested track " << url;
    return Collection::trackForUrl( url );
}

} //~ namespace

