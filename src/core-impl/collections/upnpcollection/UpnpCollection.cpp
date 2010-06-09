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
#include <kdatetime.h>
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
    , m_fullScanInProgress( false )
{
    DEBUG_BLOCK

    // experimental code, will probably be moved to a better place
    OrgKdeKDirNotifyInterface *notify = new OrgKdeKDirNotifyInterface("", "", QDBusConnection::sessionBus(), this );
    Q_ASSERT(connect( notify, SIGNAL( FilesChanged(const QStringList &) ),
                      this, SLOT( slotFilesChanged(const QStringList &) ) ));
}

UpnpCollection::~UpnpCollection()
{
}

void UpnpCollection::slotFilesChanged(const QStringList &list )
{
    if( m_fullScanInProgress )
        return;

    debug() << "Files changed" << list;
}

UpnpCollection::~UpnpCollection()
{
    // DO NOT delete m_device. It is HUpnp's job.
}

void
UpnpCollection::startFullScan()
{
    DEBUG_BLOCK;

// TODO change this to "/" when we have files changed being
/// ignored for full scans.
// right now its good to have the full scan finish quickly for
// development purposes
    startIncrementalScan( "/PC Directory/shared/music" );
    m_fullScanInProgress = true;
    m_fullScanTimer = new QTimer( this );
    Q_ASSERT(
        connect( m_fullScanTimer,
                 SIGNAL(timeout()),
                 this,
                 SLOT(updateMemoryCollection()) )
        );
    m_fullScanTimer->start(5000);
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
}

void
UpnpCollection::updateMemoryCollection()
{
    memoryCollection()->setTrackMap( m_TrackMap );
    memoryCollection()->setArtistMap( m_ArtistMap );
    memoryCollection()->setAlbumMap( m_AlbumMap );
    memoryCollection()->setGenreMap( m_GenreMap );
    memoryCollection()->setComposerMap( m_ComposerMap );
    memoryCollection()->setYearMap( m_YearMap );
    emit updated();
}

void
UpnpCollection::createTrack( const KIO::UDSEntry &entry )
{
DEBUG_BLOCK

#define INSERT_METADATA(type, value)\
    QString type##String = value;\
    Upnp##type##Ptr type( new Upnp##type( type##String ) );\
    if( m_##type##Map.contains( type##String ) )\
        type = Upnp##type##Ptr::dynamicCast( m_##type##Map[ type##String ] );\
    else\
        m_##type##Map[ type##String ] = type##Ptr::dynamicCast( type );

    // TODO check if Meta data is actually available

    INSERT_METADATA( Artist, entry.stringValue( KIO::UPNP_CREATOR ) );
    INSERT_METADATA( Album, entry.stringValue( KIO::UPNP_ALBUM ) );
    INSERT_METADATA( Genre, entry.stringValue( KIO::UPNP_GENRE ) );

    QString date = entry.stringValue( KIO::UPNP_DATE );
    KDateTime dateTime = KDateTime::fromString( date );
    int year = dateTime.date().year();
    if( !dateTime.isValid() ) {
        year = 0;
    }
    INSERT_METADATA( Year, QString::number(year) );

    Album->setAlbumArtist( Artist );
    Artist->addAlbum( Album );

    UpnpTrackPtr t( new UpnpTrack(this) ); 
    // in a recursive listing, the UDS_NAME is the relative path from
    // the base directory, so extract the filename
    QString name = entry.stringValue( KIO::UDSEntry::UDS_NAME );
    QFileInfo info(name);
    t->setTitle( info.fileName() );

    t->setArtist( Artist );
    t->setAlbum( Album );
    t->setGenre( Genre );
    t->setYear( Year );

    t->setPlayableUrl( entry.stringValue(KIO::UDSEntry::UDS_TARGET_URL) );
    t->setTrackNumber( entry.stringValue(KIO::UPNP_TRACK_NUMBER).toInt() );
// TODO validate and then convert to kbps
    t->setBitrate( entry.stringValue( KIO::UPNP_BITRATE ).toInt() / 1024 );

    QTime lengthTime = QTime::fromString( entry.stringValue( KIO::UPNP_DURATION ), Qt::ISODate );
    if( lengthTime.isValid() ) {
        qint64 msecs = lengthTime.hour() * 60 * 60 * 1000
            + lengthTime.minute() * 60 * 1000
            + lengthTime.second() * 1000
            + lengthTime.msec();
        t->setLength( msecs );
    }

    Artist->addTrack( t );
    Album->addTrack( t );
    Genre->addTrack( t );
    Year->addTrack( t );

    m_TrackMap[t->uidUrl()] = TrackPtr::dynamicCast( t );

}

void
UpnpCollection::done( KJob *job )
{
DEBUG_BLOCK
    if( job->error() ) {
        KMessageBox::error( 0, i18n("UPNP Error:") + job->errorString() );
        return;
    }
    updateMemoryCollection();
    if( m_fullScanInProgress ) {
        m_fullScanTimer->stop();
        m_fullScanInProgress = false;
        debug() << "Full Scan done";
    }
}

void
UpnpCollection::startIncrementalScan( const QString &directory )
{
    DEBUG_BLOCK;
    if( m_fullScanInProgress ) {
        debug() << "Full scan in progress, aborting";
        return;
    }
    debug() << "Scanning directory" << directory;
    KUrl url;
    url.setScheme( "upnp-ms" );
    url.setHost( m_udn );
    url.setPath( directory );
    KIO::ListJob *listJob = KIO::listRecursive( url );
    Q_ASSERT( connect( listJob, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList& )), 
                       this, SLOT(entries(KIO::Job *, const KIO::UDSEntryList&)), Qt::UniqueConnection ) );
    Q_ASSERT( connect( listJob, SIGNAL(result(KJob*)), 
                       this, SLOT(done(KJob*)), Qt::UniqueConnection ) );

}

QueryMaker*
UpnpCollection::queryMaker()
{
    DEBUG_BLOCK;
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

