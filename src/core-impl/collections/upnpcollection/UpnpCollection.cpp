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
#include "UpnpMemoryQueryMaker.h"
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
}

UpnpCollection::~UpnpCollection()
{
    // DO NOT delete m_device. It is HUpnp's job.
}

void
UpnpCollection::startFullScan()
{
    DEBUG_BLOCK
    UpnpArtistPtr artist( new UpnpArtist("Broken Social Scene") );
    UpnpAlbumPtr album( new UpnpAlbum("Forgiveness Rock Record") );
    album->setAlbumArtist( artist );

    UpnpTrackPtr t( new UpnpTrack(this) ); 
    t->setTitle( "World Sick" );
    t->setArtist( artist );
    t->setAlbum( album );

    artist->addTrack( t );
    album->addTrack( t );
    memoryCollection()->addAlbum( AlbumPtr::dynamicCast( album ) );
    memoryCollection()->addArtist( ArtistPtr::dynamicCast( artist ) );
    memoryCollection()->addTrack( TrackPtr::dynamicCast( t ) );
    //emit updated();
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
DEBUG_BLOCK
    UpnpMemoryQueryMaker *umqm = new UpnpMemoryQueryMaker(m_mc.toWeakRef(), collectionId() );
    Q_ASSERT( connect( umqm, SIGNAL(startFullScan()), this, SLOT(startFullScan()) ) );
    return umqm;
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

