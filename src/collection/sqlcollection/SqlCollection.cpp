/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>
 *  Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "SqlCollection.h"

#include "meta/capabilities/CollectionCapability.h"
#include "SqlCollectionCapability.h"
#include "DatabaseUpdater.h"
#include "Debug.h"
#include "MySqlEmbeddedCollection.h"
#include "ScanManager.h"
#include "SqlCollectionLocation.h"
#include "SqlQueryMaker.h"
#include "SqliteCollection.h"
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

AMAROK_EXPORT_PLUGIN( SqlCollectionFactory )

void
SqlCollectionFactory::init()
{
   Amarok::Collection* collection;
    /*  
    switch( CollectionDB::instance()->getDbConnectionType() )
    {
        case DbConnection::sqlite :
            collection = new SqliteCollection( "localCollection", i18n( "Local Collection" ) );
            break;
        case DbConnection::mysql :
            collection = new MySqlCollection( "localCollection", i18n( "Local Collection" ) );
            break;
        default :
            collection = new SqlCollection( "localCollection", i18n( "Local Collection" ) );
            break;
    }
    */

    collection = new MySqlEmbeddedCollection( "localCollection", i18n( "Local Collection" ) );
    emit newCollection( collection );
}

SqlCollection::SqlCollection( const QString &id, const QString &prettyName )
    : Collection()
    , m_registry( new SqlRegistry( this ) )
    , m_updater( new DatabaseUpdater( this ) )
    , m_scanManager( new ScanManager( this ) )
    , m_collectionId( id )
    , m_prettyName( prettyName )
    , m_xesamBuilder( 0 )
{
}

SqlCollection::~SqlCollection()
{
    delete m_registry;
}

void
SqlCollection::init()
{
    QTimer::singleShot( 0, this, SLOT( initXesam() ) );
    if( m_updater->needsUpdate() )
        m_updater->update();
    QStringList result = query( "SELECT count(*) FROM tracks" );
    // If database version is updated, the collection needs to be rescanned.
    // Works also if the collection is empty for some other reason
    // (e.g. deleted collection.db)
    if( !result.isEmpty() && result.first().toInt() == 0 )
    {
        QTimer::singleShot( 0, m_scanManager, SLOT( startFullScan() ) );
    }
    //perform a quick check of the database
    m_updater->cleanupDatabase();
}

void
SqlCollection::startFullScan()
{
    m_scanManager->startFullScan();
}

void
SqlCollection::startIncrementalScan()
{
    m_scanManager->startIncrementalScan();
}

void
SqlCollection::stopScan()
{
    DEBUG_BLOCK

    delete m_scanManager;
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
    return new SqlQueryMaker( this );
}

SqlRegistry*
SqlCollection::registry() const
{
    return m_registry;
}

DatabaseUpdater*
SqlCollection::dbUpdater() const
{
    return m_updater;
}

ScanManager*
SqlCollection::scanManager() const
{
    return m_scanManager;
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
    return new SqlCollectionLocation( this );
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

QString
SqlCollection::escape( QString text ) const           //krazy:exclude=constref
{
    return text.replace( '\'', "''" );;
}


int
SqlCollection::sqlDatabasePriority() const
{
    return 1;
}

QString
SqlCollection::type() const
{
    return "sql";
}

QString
SqlCollection::boolTrue() const
{
    return "1";
}

QString
SqlCollection::boolFalse() const
{
    return "0";
}

QString
SqlCollection::idType() const
{
    return "INTEGER PRIMARY KEY AUTO_INCREMENT";
}

QString
SqlCollection::textColumnType( int length ) const
{
    return QString( "VARCHAR(%1)" ).arg( length );
}

QString
SqlCollection::exactTextColumnType( int length ) const
{
    return textColumnType( length );
}

QString
SqlCollection::longTextColumnType() const
{
    return "TEXT";
}

QString
SqlCollection::randomFunc() const
{
    return "RANDOM()";
}

void
SqlCollection::vacuum() const
{
    //implement in subclasses if necessary
}

void
SqlCollection::updateTrackUrls( TrackUrls changedUrls ) //SLOT
{
    DEBUG_BLOCK

    foreach( const QString &key, changedUrls.keys() )
    {
        if( m_registry->checkUidExists( key ) )
        {
            Meta::TrackPtr track = m_registry->getTrackFromUid( key );
            if( track )
                KSharedPtr<Meta::SqlTrack>::staticCast( track )->setUrl( changedUrls[key] );
        }
    }
}

void
SqlCollection::initXesam() //SLOT
{
    m_xesamBuilder = new XesamCollectionBuilder( this );
}

bool
SqlCollection::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    DEBUG_BLOCK
    switch( type )
    {
        case Meta::Capability::Collection:
            return true;
        default:
            return false;
    }
}

Meta::Capability*
SqlCollection::asCapabilityInterface( Meta::Capability::Type type )
{
    DEBUG_BLOCK
    switch( type )
    {
        case Meta::Capability::Collection:
            return new Meta::SqlCollectionCapability( this );
        default:
            return 0;
    }
}

void
SqlCollection::deleteTracksSlot( Meta::TrackList tracklist )
{
    DEBUG_BLOCK
    QStringList files;
    foreach( Meta::TrackPtr track, tracklist )
        files << track->prettyUrl();

    // TODO put the delete confirmation code somewhere else?
    const QString text( i18nc( "@info", "Do you really want to delete these %1 tracks? They will be removed from disk as well as your collection.", tracklist.count() ) );
    const bool del = KMessageBox::warningContinueCancelList(0,
                                                     text,
                                                     files,
                                                     i18n("Delete Files"),
                                                     KStandardGuiItem::del() ) == KMessageBox::Yes;
    if( !del )
        return;

	CollectionLocation *loc = location();
    // remove the tracks from the collection maps
    //TODO make unblocking
    foreach( Meta::TrackPtr track, tracklist )
        loc->remove( track );

    loc->deleteLater();
    // inform treeview collection has updated
    emit updated();
}


#include "SqlCollection.moc"

