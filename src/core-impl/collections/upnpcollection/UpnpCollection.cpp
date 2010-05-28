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

#define DEBUG_PREFIX "UpnpCollection"

#include "UpnpCollection.h"

#include "core/support/Debug.h"
#include "MemoryQueryMaker.h"
#include "statusbar/StatusBar.h"
#include "UpnpQueryMaker.h"
#include "UpnpMeta.h"

#include <QStringList>
#include <QTimer>

#include <KLocale>

using namespace Meta;

namespace Collections {

//UpnpCollection

// TODO register for the device bye bye and emit remove()
    UpnpCollection::UpnpCollection( const QString &udn, const QString &name )
    : Collection()
    , m_udn( udn )
    , m_name( name )
    , m_mc( new MemoryCollection() )
{
    DEBUG_BLOCK
        emit updated();
    UpnpTrackPtr t( new UpnpTrack(this) ); 
    t->setTitle( "World Sick" );
    m_mc->addTrack( TrackPtr::dynamicCast( t ) );
}

UpnpCollection::~UpnpCollection()
{
    // DO NOT delete m_device. It is HUpnp's job.
}

void
UpnpCollection::startFullScan()
{
    DEBUG_BLOCK
    //ignore
}

void
UpnpCollection::startIncrementalScan( const QString &directory )
{
DEBUG_BLOCK
    debug() << "Scanning directory" << directory;
}

QueryMaker*
UpnpCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
UpnpCollection::collectionId() const
{
// TODO
    return QString("upnp-ms://") + m_udn;
}

QString
UpnpCollection::prettyName() const
{
// TODO
    return m_name;
}

bool
UpnpCollection::possiblyContainsTrack( const KUrl &url ) const
{
    debug() << "Requested track " << url;
    return false;
}

} //~ namespace
#include "UpnpCollection.moc"

