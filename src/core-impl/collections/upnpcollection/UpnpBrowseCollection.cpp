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

#define DEBUG_PREFIX "UpnpBrowseCollection"

#include "UpnpBrowseCollection.h"

#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "MemoryQueryMaker.h"

#include "UpnpMemoryQueryMaker.h"
#include "UpnpQueryMaker.h"
#include "UpnpMeta.h"
#include "UpnpCache.h"
#include "upnptypes.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTimer>

#include <KLocalizedString>
#include <KIO/Scheduler>
#include <KIO/ListJob>

using namespace Meta;

namespace Collections {

//UpnpBrowseCollection

// TODO register for the device bye bye and emit remove()
UpnpBrowseCollection::UpnpBrowseCollection( const DeviceInfo& dev )
    : UpnpCollectionBase( dev )
    , m_mc( new MemoryCollection() )
    , m_fullScanInProgress( false )
    , m_cache( new UpnpCache( this ) )
{
    DEBUG_BLOCK

    // experimental code, will probably be moved to a better place
    OrgKdeKDirNotifyInterface *notify = new OrgKdeKDirNotifyInterface("", "", QDBusConnection::sessionBus(), this );
    connect( notify, &OrgKdeKDirNotifyInterface::FilesChanged, this, &UpnpBrowseCollection::slotFilesChanged );
}

UpnpBrowseCollection::~UpnpBrowseCollection()
{
}

void UpnpBrowseCollection::slotFilesChanged(const QStringList &list )
{
    if( m_fullScanInProgress )
        return;

    m_updateQueue += list;

    debug() << "Files changed" << list;
}

void UpnpBrowseCollection::processUpdates()
{
    if( m_updateQueue.isEmpty() )
        return;

    QString urlString = m_updateQueue.dequeue();
    debug() << "Update URL is" << urlString;
    invalidateTracksIn( urlString );
    QUrl url( urlString );
    if( url.scheme() != "upnp-ms" || m_device.uuid() != url.host() )
        return;
    debug() << "Now incremental scanning" << url;
    startIncrementalScan( url.path() );
}

void UpnpBrowseCollection::invalidateTracksIn( const QString &dir )
{
    debug() << "INVALIDATING" << m_tracksInContainer[dir].length();

    /*
     * when we get dir as / a / b we also have to invalidate
     * any tracks in / a / b / * so we need to iterate over keys
     * If performance is really affected we can use some
     * kind of a prefix tree instead of a hash.
     */
    foreach( const QString &key, m_tracksInContainer.keys() ) {
        if( key.startsWith( dir ) ) {
            debug() << key << " matches " << dir;
            foreach( TrackPtr track, m_tracksInContainer[dir] ) {
                removeTrack( track );
            }
        }
    }
    m_tracksInContainer.remove( dir );
}

void
UpnpBrowseCollection::startFullScan()
{
    DEBUG_BLOCK;

    // TODO probably set abort slot
    // TODO figure out what to do with the total steps
    Amarok::Logger::newProgressOperation( this, i18n( "Scanning %1", prettyName() ) );

    startIncrementalScan( "/" );

    m_fullScanInProgress = true;
    m_fullScanTimer = new QTimer( this );
    connect( m_fullScanTimer, &QTimer::timeout,
             this, &UpnpBrowseCollection::updateMemoryCollection );
    m_fullScanTimer->start(5000);
}

void
UpnpBrowseCollection::startIncrementalScan( const QString &directory )
{
    if( m_fullScanInProgress ) {
        debug() << "Full scan in progress, aborting";
        return;
    }
    debug() << "Scanning directory" << directory;
    QUrl url;
    url.setScheme( "upnp-ms" );
    url.setHost( m_device.uuid() );
    url.setPath( directory );
    KIO::ListJob *listJob = KIO::listRecursive( url, KIO::HideProgressInfo );
    addJob( listJob );
    Q_ASSERT( connect( listJob, &KIO::ListJob::entries, this, &UpnpBrowseCollection::entries, Qt::UniqueConnection ) );
    Q_ASSERT( connect( listJob, &KJob::result, this, &UpnpBrowseCollection::done, Qt::UniqueConnection ) );
    listJob->start();

}

void
UpnpBrowseCollection::entries( KIO::Job *job, const KIO::UDSEntryList &list )
{
    DEBUG_BLOCK;
    int count = 0;
    KIO::SimpleJob *sj = static_cast<KIO::SimpleJob *>( job );
    foreach( const KIO::UDSEntry &entry, list ) {
        if( entry.contains( KIO::UPNP_CLASS )
            && entry.stringValue( KIO::UPNP_CLASS ).startsWith( "object.item.audioItem" ) ) {
            createTrack( entry, sj->url().toDisplayString() );
        }
        count++;
        emit totalSteps( count );
        emit incrementProgress();
    }
    updateMemoryCollection();
}

void
UpnpBrowseCollection::updateMemoryCollection()
{
    memoryCollection()->setTrackMap( m_cache->tracks() );
    memoryCollection()->setArtistMap( m_cache->artists() );
    memoryCollection()->setAlbumMap( m_cache->albums() );
    memoryCollection()->setGenreMap( m_cache->genres() );
    memoryCollection()->setYearMap( m_cache->years() );
    emit updated();
}

void
UpnpBrowseCollection::createTrack( const KIO::UDSEntry &entry, const QString &baseUrl )
{
DEBUG_BLOCK
    TrackPtr t = m_cache->getTrack( entry );

    QFileInfo info( entry.stringValue( KIO::UDSEntry::UDS_NAME ) );
    QString container = QDir(baseUrl).filePath( info.dir().path() );
    debug() << "CONTAINER" << container;
    m_tracksInContainer[container] << t;
}

void
UpnpBrowseCollection::removeTrack( TrackPtr t )
{
    m_cache->removeTrack( t );
}

void
UpnpBrowseCollection::done( KJob *job )
{
DEBUG_BLOCK
    if( job->error() )
    {
        Amarok::Logger::longMessage( i18n("UPnP Error: %1", job->errorString() ),
                                                   Amarok::Logger::Error );
        return;
    }
    updateMemoryCollection();
    if( m_fullScanInProgress )
    {
        m_fullScanTimer->stop();
        m_fullScanInProgress = false;
        emit endProgressOperation( this );
        debug() << "Full Scan done";
    }

    // process new updates if any
    // this is the only place processUpdates()
    // should be called since a full scan at the very beginning
    // will always call done().
    processUpdates();
}

bool
UpnpBrowseCollection::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    return ( type == Capabilities::Capability::CollectionScan );
}

Capabilities::Capability*
UpnpBrowseCollection::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( type == Capabilities::Capability::CollectionScan )
        return new UpnpBrowseCollectionScanCapability( this );
    else
        return 0;
}

QueryMaker*
UpnpBrowseCollection::queryMaker()
{
    DEBUG_BLOCK;
    UpnpMemoryQueryMaker *umqm = new UpnpMemoryQueryMaker(m_mc.toWeakRef(), collectionId() );
    connect( umqm, &UpnpMemoryQueryMaker::startFullScan, this, &UpnpBrowseCollection::startFullScan );
    return umqm;
}

Meta::TrackPtr
UpnpBrowseCollection::trackForUrl( const QUrl &url )
{
    debug() << "TRACK FOR URL " << url;
    if( url.scheme() == "upnptrack" && url.host() == collectionId() )
        return m_cache->tracks()[url.url()];
    debug() << "NONE FOUND";
    return Collection::trackForUrl( url );
}

// ---------- CollectionScanCapability ------------

UpnpBrowseCollectionScanCapability::UpnpBrowseCollectionScanCapability( UpnpBrowseCollection* collection )
    : m_collection( collection )
{ }

UpnpBrowseCollectionScanCapability::~UpnpBrowseCollectionScanCapability()
{ }

void
UpnpBrowseCollectionScanCapability::startFullScan()
{
    m_collection->startFullScan();
}

void
UpnpBrowseCollectionScanCapability::startIncrementalScan( const QString &directory )
{
    m_collection->startIncrementalScan( directory );
}

void
UpnpBrowseCollectionScanCapability::stopScan()
{
    // the UpnpBrowseCollection does not yet know how to stop a scan
}

} //~ namespace

