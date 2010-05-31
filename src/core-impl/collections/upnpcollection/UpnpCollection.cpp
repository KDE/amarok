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
#include <KMessageBox>
#include <kio/upnptypes.h>
#include <kio/scheduler.h>
#include <kio/jobclasses.h>

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

        KIO::ListJob *job = KIO::listDir(KUrl("upnp-ms://bf7eace9-e63f-4267-a871-7b572d750653/PC Directory/shared/music/zero project/Fairytale"));
    Q_ASSERT( connect( job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList& )), 
                       this, SLOT(entries(KIO::Job *, const KIO::UDSEntryList&)) ) );
    Q_ASSERT( connect( job, SIGNAL(result(KJob*)), 
                       this, SLOT(done(KJob*)) ) );

}

void
UpnpCollection::entries( KIO::Job *job, const KIO::UDSEntryList &list )
{
DEBUG_BLOCK
    foreach( KIO::UDSEntry entry, list ) {
        if( entry.contains( KIO::UPNP_CLASS )
            && entry.stringValue( KIO::UPNP_CLASS ) == "object.item.audioItem.musicTrack" ) {
            createTrack( entry );
        }
    }

    memoryCollection()->setArtistMap( m_artistMap );
    memoryCollection()->setAlbumMap( m_albumMap );
    emit updated();
}

void
UpnpCollection::createTrack( const KIO::UDSEntry &entry )
{
DEBUG_BLOCK
    debug() << "CREATING TRACK";
    // TODO check if Meta data is actually available

    QString artistName = entry.stringValue(KIO::UPNP_CREATOR);
    UpnpArtistPtr artist( new UpnpArtist( artistName ) );
    if( m_artistMap.contains( artistName ) )
        artist = UpnpArtistPtr::dynamicCast( m_artistMap[artistName] );
    else
        m_artistMap[artistName] = ArtistPtr::dynamicCast( artist );

    QString albumName = entry.stringValue(KIO::UPNP_ALBUM);
    UpnpAlbumPtr album( new UpnpAlbum(albumName) );
    if( m_albumMap.contains( albumName ) )
        album = UpnpAlbumPtr::dynamicCast( m_albumMap[albumName] );
    else
        m_albumMap[albumName] = AlbumPtr::dynamicCast( album );

    album->setAlbumArtist( artist );
    artist->addAlbum( album );

    UpnpTrackPtr t( new UpnpTrack(this) ); 
    t->setTitle( entry.stringValue(KIO::UDSEntry::UDS_NAME) );
    t->setArtist( artist );
    t->setAlbum( album );
    t->setYear( UpnpYearPtr( new UpnpYear( "2010" ) ) );
    t->setPlayableUrl( entry.stringValue(KIO::UDSEntry::UDS_TARGET_URL) );
    t->setTrackNumber( entry.stringValue(KIO::UPNP_TRACK_NUMBER).toInt() );

    artist->addTrack( t );
    album->addTrack( t );

    memoryCollection()->addTrack( TrackPtr::dynamicCast( t ) );

}

void
UpnpCollection::done( KJob *job )
{
DEBUG_BLOCK
    if( job->error() ) {
        KMessageBox::error( 0, i18n("UPNP Error:") + job->errorString() );
    }
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

