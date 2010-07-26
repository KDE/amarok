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

#define DEBUG_PREFIX "UpnpCollectionBase"

#include "UpnpCollectionBase.h"

#include <kio/upnptypes.h>
#include <kio/scheduler.h>
#include <kio/jobclasses.h>

namespace KIO {
  class Job;
  class ListJob;
}
class KJob;

namespace Collections {

UpnpCollectionBase::UpnpCollectionBase( Solid::Device dev )
    : Collection()
    , m_device( dev )
{
}

QString UpnpCollectionBase::collectionId() const
{
    return QString("upnp-ms://") + m_device.udi().replace("/org/kde/upnp/uuid:", "");
}

QString UpnpCollectionBase::prettyName() const
{
    return m_device.product();
}

bool UpnpCollectionBase::possiblyContainsTrack( const KUrl &url ) const
{
    debug() << "CONTAINS?" << url;
    if( url.scheme() == "upnp-ms" )
//         && url.host() == m_device.host()
//         && url.port() == m_device.port() )
        return true;
    return false;
}

} //namespace Collections
