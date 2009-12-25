/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
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

#include "SqlCollection.h"

#include "CapabilityDelegate.h"
#include "DatabaseUpdater.h"
#include "Debug.h"
#include "ScanManager.h"
#include "SqlCollectionLocation.h"
#include "SqlQueryMaker.h"
#include "SvgHandler.h"

#ifdef Q_OS_WIN32
class XesamCollectionBuilder
{
public:
    XesamCollectionBuilder(SqlCollection *collection) {}
};
#else
#include "XesamCollectionBuilder.h"
#endif

#include <klocale.h>
#include <KIcon>
#include <KMessageBox> // TODO put the delete confirmation code somewhere else?
#include <QTimer>

SqlCollection::SqlCollection( const QString &id, const QString &prettyName )
    : Collection()
    , m_registry( 0 )
    , m_updater( 0 )
    , m_capabilityDelegate( 0 )
    , m_sqlStorage( 0 )
    , m_collectionLocationFactory( 0 )
    , m_queryMakerFactory( 0 )
    , m_scanManager( 0 )
    , m_mpm( 0 )
    , m_collectionId( id )
    , m_prettyName( prettyName )
    , m_xesamBuilder( 0 )
{
    qRegisterMetaType<TrackUrls>( "TrackUrls" );
    qRegisterMetaType<ChangedTrackUrls>( "ChangedTrackUrls" );
}

SqlCollection::~SqlCollection()
{
    delete m_registry;
    delete m_capabilityDelegate;
    delete m_updater;
    delete m_collectionLocationFactory;
    delete m_queryMakerFactory;
    delete m_sqlStorage;
    //do not delete m_mpm (yet)
}

void
SqlCollection::init()
{
    QTimer::singleShot( 0, this, SLOT( initXesam() ) );
    if( !m_updater )
    {
        debug() << "Could not load updater!";
        return;
    }

    if( m_updater->needsUpdate() )
        m_updater->update();

    QStringList result = m_sqlStorage->query( "SELECT count(*) FROM tracks" );
    // If database version is updated, the collection needs to be rescanned.
    // Works also if the collection is empty for some other reason
    // (e.g. deleted collection.db)
    if( m_updater->rescanNeeded() || ( !result.isEmpty() && result.first().toInt() == 0 ) )
    {
        QTimer::singleShot( 0, m_scanManager, SLOT( startFullScan() ) );
    }
    //perform a quick check of the database
    m_updater->cleanupDatabase();
}

void
SqlCollection::startFullScan()
{
    if( m_scanManager )
        m_scanManager->startFullScan();
}

void
SqlCollection::startIncrementalScan()
{
    if( m_scanManager )
        m_scanManager->startIncrementalScan();
}

void
SqlCollection::stopScan()
{
    DEBUG_BLOCK

    if( m_scanManager )
        m_scanManager->abort( "Abort requested from SqlCollection::stopScan()" );
}

QString
SqlCollection::uidUrlProtocol() const
{
    return "amarok-sqltrackuid";
}

QString
SqlCollection::collectionId() const
{
    return m_collectionId;
}

QString
SqlCollection::prettyName() const
{
    return m_prettyName;
}

QueryMaker*
SqlCollection::queryMaker()
{
    Q_ASSERT( m_queryMakerFactory );
    return m_queryMakerFactory->createQueryMaker();
}

SqlRegistry*
SqlCollection::registry() const
{
    Q_ASSERT( m_registry );
    return m_registry;
}

DatabaseUpdater*
SqlCollection::dbUpdater() const
{
    Q_ASSERT( m_updater );
    return m_updater;
}

ScanManager*
SqlCollection::scanManager() const
{
    Q_ASSERT( m_scanManager );
    return m_scanManager;
}

void
SqlCollection::setScanManager( ScanManager *manager )
{
    m_scanManager = manager;
}

SqlStorage*
SqlCollection::sqlStorage() const
{
    Q_ASSERT( m_sqlStorage );
    return m_sqlStorage;
}

SqlMountPointManager*
SqlCollection::mountPointManager() const
{
    Q_ASSERT( m_mpm );
    return m_mpm;
}

void
SqlCollection::removeCollection()
{
    emit remove();
}

bool
SqlCollection::isDirInCollection( QString path )
{
    return m_scanManager->isDirInCollection( path );
}

bool
SqlCollection::isFileInCollection( const QString &url )
{
    return m_scanManager->isFileInCollection( url );
}

bool
SqlCollection::possiblyContainsTrack( const KUrl &url ) const
{
    return url.protocol() == "file" || url.protocol() == uidUrlProtocol();
}

Meta::TrackPtr
SqlCollection::trackForUrl( const KUrl &url )
{
    if( url.protocol() == uidUrlProtocol() )
        return m_registry->getTrackFromUid( url.url() );
    return m_registry->getTrack( url.path() );
}

CollectionLocation*
SqlCollection::location() const
{
    Q_ASSERT( m_collectionLocationFactory );
    return m_collectionLocationFactory->createSqlCollectionLocation();
}

bool
SqlCollection::isWritable() const
{
    return true;
}

bool
SqlCollection::isOrganizable() const
{
    return true;
}

void
SqlCollection::sendChangedSignal()
{
    emit updated();
}

void
SqlCollection::updateTrackUrlsUids( const ChangedTrackUrls &changedUrls, const QHash<QString, QString> &changedUids ) //SLOT
{
    DEBUG_BLOCK
    QHash<QString, KSharedPtr<Meta::SqlTrack> > trackList; //dun want duplicates
    foreach( const QString &key, changedUrls.keys() )
    {
        if( m_registry->checkUidExists( key ) )
        {
            m_registry->updateCachedUrl( changedUrls.value( key ) );
            Meta::TrackPtr track = m_registry->getTrackFromUid( key );
            if( track )
                trackList.insert( key, KSharedPtr<Meta::SqlTrack>::staticCast( track ) );
        }
    }
    foreach( const QString &key, changedUids.keys() )
    {
        //old uid is key, new is value
        if( m_registry->checkUidExists( key ) )
        {
            m_registry->updateCachedUid( key, changedUids[key] );
            Meta::TrackPtr track = m_registry->getTrackFromUid( key );
            if( track )
                trackList.insert( changedUids[key], KSharedPtr<Meta::SqlTrack>::staticCast( track ) );
        }
    }
    foreach( QString key, trackList.keys() )
        trackList[key]->refreshFromDatabase( key, this, true );
}

void
SqlCollection::initXesam() //SLOT
{
    m_xesamBuilder = new XesamCollectionBuilder( this );
}

bool
SqlCollection::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    return ( m_capabilityDelegate ? m_capabilityDelegate->hasCapabilityInterface( type, this ) : false );
}

Meta::Capability*
SqlCollection::createCapabilityInterface( Meta::Capability::Type type )
{
    return ( m_capabilityDelegate ? m_capabilityDelegate->createCapabilityInterface( type, this ) : 0 );
}

void
SqlCollection::deleteTracksSlot( Meta::TrackList tracklist )
{
    DEBUG_BLOCK
    QStringList files;
    foreach( Meta::TrackPtr track, tracklist )
        files << track->prettyUrl();

	CollectionLocation *loc = location();
    // remove the tracks from the collection maps
    //TODO make unblocking
    foreach( Meta::TrackPtr track, tracklist )
        loc->remove( track );

    loc->deleteLater();
    // inform treeview collection has updated
    emit updated();
}

void
SqlCollection::dumpDatabaseContent()
{
    QStringList tables = m_sqlStorage->query( "select table_name from INFORMATION_SCHEMA.tables WHERE table_schema='amarok'" );
    foreach( const QString &table, tables )
    {
        m_updater->writeCSVFile( table, table, true );
    }
}

#include "SqlCollection.moc"

