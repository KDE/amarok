/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#define DEBUG_PREFIX "CollectionManager"

#include "CollectionManager.h"

#include "Debug.h"

#include "Collection.h"
#include "EngineController.h"
#include "MetaQueryMaker.h"
#include "core/meta/impl/file/File.h"
#include "core/meta/impl/stream/Stream.h"
#include "PluginManager.h"
#include "SmartPointerList.h"
#include "SqlStorage.h"
#include "core/meta/impl/timecode/TimecodeTrackProvider.h"

#include <QList>
#include <QMetaEnum>
#include <QMetaObject>
#include <QPair>
#include <QTimer>

#include <KBuildSycocaProgressDialog>
#include <KGlobal>
#include <KMessageBox>
#include <KService>
#include <KPluginLoader>
#include <KPluginFactory>

#include <cstdlib>

typedef QPair<Amarok::Collection*, CollectionManager::CollectionStatus> CollectionPair;

class SqlStorageWrapper : public SqlStorage
{
public:
    SqlStorageWrapper()
        : SqlStorage()
        , m_sqlStorage( 0 )
    {}

    virtual int sqlDatabasePriority() const { return ( m_sqlStorage ? m_sqlStorage->sqlDatabasePriority() : 0 ); }
    virtual QString type() const  { return ( m_sqlStorage ? m_sqlStorage->type() : "SqlStorageWrapper" ); }
    virtual QString escape( QString text ) const  { return ( m_sqlStorage ? m_sqlStorage->escape( text ) : text ); }
    virtual QStringList query( const QString &query )  { return ( m_sqlStorage ? m_sqlStorage->query( query ) : QStringList() ); }
    virtual int insert( const QString &statement, const QString &table )  { return ( m_sqlStorage ? m_sqlStorage->insert( statement, table ) : 0 ); }
    virtual QString boolTrue() const  { return ( m_sqlStorage ? m_sqlStorage->boolTrue() : "1" ); }
    virtual QString boolFalse() const  { return ( m_sqlStorage ? m_sqlStorage->boolFalse() : "0" ); }
    virtual QString idType() const  { return ( m_sqlStorage ? m_sqlStorage->idType() : "WRAPPER_NOT_IMPLEMENTED" ); }
    virtual QString textColumnType( int length ) const { return ( m_sqlStorage ? m_sqlStorage->textColumnType( length ) : "WRAPPER_NOT_IMPLEMENTED" ); }
    virtual QString exactTextColumnType( int length ) const { return ( m_sqlStorage ? m_sqlStorage->exactTextColumnType( length ) : "WRAPPER_NOT_IMPLEMENTED" ); }
    virtual QString exactIndexableTextColumnType( int length ) const { return ( m_sqlStorage ? m_sqlStorage->exactIndexableTextColumnType( length ) : "WRAPPER_NOT_IMPLEMENTED" ); };
    virtual QString longTextColumnType() const { return ( m_sqlStorage ? m_sqlStorage->longTextColumnType() : "WRAPPER_NOT_IMPLEMENTED" ); }
    virtual QString randomFunc() const { return ( m_sqlStorage ? m_sqlStorage->randomFunc() : "WRAPPER_NOT_IMPLEMENTED" ); }

    void setSqlStorage( SqlStorage *sqlStorage ) { m_sqlStorage = sqlStorage; }
private:
    SqlStorage *m_sqlStorage;
};

struct CollectionManager::Private
{
    QList<CollectionPair> collections;
    SmartPointerList<Amarok::CollectionFactory> factories;
    SqlStorage *sqlDatabase;
    SqlStorageWrapper *sqlStorageWrapper;
    QList<Amarok::Collection*> unmanagedCollections;
    SmartPointerList<Amarok::Collection> managedCollections;
    QList<Amarok::TrackProvider*> trackProviders;
    Amarok::Collection *primaryCollection;
};

CollectionManager* CollectionManager::s_instance = 0;

CollectionManager *
CollectionManager::instance()
{
    return s_instance ? s_instance : new CollectionManager();
}

void
CollectionManager::destroy()
{
    if (s_instance) {
        delete s_instance;
        s_instance = 0;
    }
}

CollectionManager::CollectionManager()
    : QObject()
    , d( new Private )
{
    qRegisterMetaType<SqlStorage *>( "SqlStorage*" );
    d->sqlDatabase = 0;
    d->primaryCollection = 0;
    d->sqlStorageWrapper = new SqlStorageWrapper();
    s_instance = this;
    m_haveEmbeddedMysql = false;

    init();
}

CollectionManager::~CollectionManager()
{
    DEBUG_BLOCK

    //not deleting SqlStorageWrapper here as somebody might be caching it
    //Amarok really needs a proper state management...
    d->sqlStorageWrapper->setSqlStorage( 0 );
    delete m_timecodeTrackProvider;
    d->collections.clear();
    d->unmanagedCollections.clear();
    d->trackProviders.clear();
    qDeleteAll( d->managedCollections );
    qDeleteAll( d->factories );


    delete d;
}

void
CollectionManager::init()
{
    DEBUG_BLOCK

    //register the timecode track provider now, as it needs to get added before loading
    //the stored playlist... Since it can have playable urls that might also match other providers, it needs to get added first.
    m_timecodeTrackProvider = new TimecodeTrackProvider();
    addTrackProvider( m_timecodeTrackProvider );

    KService::List plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'collection'" );
    debug() << "Received [" << QString::number( plugins.count() ) << "] collection plugin offers";

    if( plugins.isEmpty() )
    {
        debug() << "No Amarok plugins found, running kbuildsycoca4.";
        KBuildSycocaProgressDialog::rebuildKSycoca( 0 );

        plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'collection'" );
        debug() << "Second attempt: Received [" << QString::number( plugins.count() ) << "] collection plugin offers";

        if( plugins.isEmpty() )
        {
            KMessageBox::error( 0, i18n(
                    "<p>Amarok could not find any collection plugins. "
                    "It is possible that Amarok is installed under the wrong prefix, please fix your installation using:<pre>"
                    "$ cd /path/to/amarok/source-code/<br>"
                    "$ su -c \"make uninstall\"<br>"
                    "$ cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` && su -c \"make install\"<br>"
                    "$ kbuildsycoca4 --noincremental<br>"
                    "$ amarok</pre>"
                    "More information can be found in the README file. For further assistance join us at #amarok on irc.freenode.net.</p>" ) );
            // don't use QApplication::exit, as the eventloop may not have started yet
            std::exit( EXIT_SUCCESS );
        }
    }

    foreach( KService::Ptr service, plugins )
    {
        const QString name = service->property( "X-KDE-Amarok-name" ).toString();
        if( name == "mysqlserver-collection" &&
           !Amarok::config( "MySQL" ).readEntry( "UseServer", false ) )
                continue;
        if( name == "mysqle-collection" &&
            Amarok::config( "MySQL" ).readEntry( "UseServer", false ) )
                continue;
        if( name == "mysqle-collection" || name == "mysqlserver-collection" )
        {
            KPluginLoader loader( *( service.constData() ) );
            KPluginFactory *pluginFactory = loader.factory();
            if ( pluginFactory )
            {
                Amarok::CollectionFactory* factory = pluginFactory->create<Amarok::CollectionFactory>( this );
                if ( factory )
                {
                    debug() << "Initialising sqlcollection";
                    connect( factory, SIGNAL( newCollection( Amarok::Collection* ) ), this, SLOT( slotNewCollection( Amarok::Collection* ) ) );
                    d->factories.append( factory );
                    factory->init();
                    if( name == "mysqle-collection" )
                        m_haveEmbeddedMysql = true;
                }
                else
                {
                    debug() << "SqlCollection Plugin has wrong factory class: " << loader.errorString();
                }
            }
            else
            {
                warning() << "Failed to get factory from KPluginLoader: " << loader.errorString();
            }
            break;
        }
    }

    foreach( KService::Ptr service, plugins )
    {
        //ignore sqlcollection plugins, we have already loaded it above
        if( service->property( "X-KDE-Amarok-name" ).toString() == "mysqlserver-collection" )
                continue;
        if( service->property( "X-KDE-Amarok-name" ).toString() == "mysqle-collection" )
                continue;

        KPluginLoader loader( *( service.constData() ) );
        KPluginFactory *pluginFactory = loader.factory();
        if ( pluginFactory )
        {
            Amarok::CollectionFactory* factory = pluginFactory->create<Amarok::CollectionFactory>( this );
            if ( factory )
            {
                connect( factory, SIGNAL( newCollection( Amarok::Collection* ) ), this, SLOT( slotNewCollection( Amarok::Collection* ) ) );
                d->factories.append( factory );
                factory->init();
            }
            else
            {
                debug() << "Plugin has wrong factory class: " << loader.errorString();
                continue;
            }
        }
        else
        {
            warning() << "Failed to get factory from KPluginLoader: " << loader.errorString();
        }
    }


}

void
CollectionManager::startFullScan()
{
    foreach( const CollectionPair &pair, d->collections )
    {
        pair.first->startFullScan();
    }
}

void
CollectionManager::startIncrementalScan( const QString &directory )
{
    foreach( const CollectionPair &pair, d->collections )
    {
        pair.first->startIncrementalScan( directory );
    }
}

void
CollectionManager::stopScan()
{
    foreach( const CollectionPair &pair, d->collections )
    {
        pair.first->stopScan();
    }
}

void
CollectionManager::checkCollectionChanges()
{
    DEBUG_BLOCK

    foreach( const CollectionPair &pair, d->collections )
    {
        pair.first->startIncrementalScan();
    }
}

QueryMaker*
CollectionManager::queryMaker() const
{
    QList<Amarok::Collection*> colls;
    foreach( const CollectionPair &pair, d->collections )
    {
        if( pair.second & CollectionQueryable )
        {
            colls << pair.first;
        }
    }
    return new MetaQueryMaker( colls );
}

void
CollectionManager::slotNewCollection( Amarok::Collection* newCollection )
{
    DEBUG_BLOCK
    if( !newCollection )
    {
        debug() << "Warning, newCollection in slotNewCollection is 0";
        return;
    }
    foreach( CollectionPair p, d->collections )
    {
        if( p.first == newCollection )
        {
            debug() << "Warning, newCollection is already being managed";
            return;
        }
    }

    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "CollectionStatus" ) );
    const QString &value = KGlobal::config()->group( "CollectionManager" ).readEntry( newCollection->collectionId() );
    int enumValue = me.keyToValue( value.toLocal8Bit().constData() );
    CollectionStatus status;
    enumValue == -1 ? status = CollectionEnabled : status = (CollectionStatus) enumValue;
    CollectionPair pair( newCollection, status );
    d->collections.append( pair );
    d->managedCollections.append( newCollection );
    d->trackProviders.append( newCollection );
    connect( newCollection, SIGNAL( remove() ), SLOT( slotRemoveCollection() ), Qt::QueuedConnection );
    connect( newCollection, SIGNAL( updated() ), SLOT( slotCollectionChanged() ), Qt::QueuedConnection );
    //by convention, collections that provide a SQL database have a Qt property called "sqlStorage"
    int propertyIndex = newCollection->metaObject()->indexOfProperty( "sqlStorage" );
    if( propertyIndex != -1 )
    {
        SqlStorage *sqlStorage = newCollection->property( "sqlStorage" ).value<SqlStorage*>();
        if( sqlStorage )
        {
            //let's cheat a bit and assume that sqlStorage and the primaryCollection are always the same
            //it is true for now anyway
            if( d->sqlDatabase )
            {
                if( d->sqlDatabase->sqlDatabasePriority() < sqlStorage->sqlDatabasePriority() )
                {
                    d->sqlDatabase = sqlStorage;
                    d->primaryCollection = newCollection;
                    d->sqlStorageWrapper->setSqlStorage( sqlStorage );
                }
            }
            else
            {
                d->sqlDatabase = sqlStorage;
                d->primaryCollection = newCollection;
                d->sqlStorageWrapper->setSqlStorage( sqlStorage );
            }
        }
        else
        {
            warning() << "Collection " << newCollection->collectionId() << " has sqlStorage property but did not provide a SqlStorage pointer";
        }
    }
    if( status & CollectionViewable )
    {
        emit collectionAdded( newCollection );
        emit collectionAdded( newCollection, status );
    }
}

void
CollectionManager::slotRemoveCollection()
{
    Amarok::Collection* collection = qobject_cast<Amarok::Collection*>( sender() );
    if( collection )
    {
        CollectionStatus status = collectionStatus( collection->collectionId() );
        CollectionPair pair( collection, status );
        d->collections.removeAll( pair );
        d->managedCollections.removeAll( collection );
        d->trackProviders.removeAll( collection );
        QVariant v = collection->property( "sqlStorage" );
        if( v.isValid() )
        {
            SqlStorage *sqlDb = v.value<SqlStorage*>();
            if( sqlDb && sqlDb == d->sqlDatabase )
            {
                SqlStorage *newSqlDatabase = 0;
                foreach( const CollectionPair &pair, d->collections )
                {
                    QVariant variant = pair.first->property( "sqlStorage" );
                    if( !variant.isValid() )
                        continue;
                    SqlStorage *sqlDb = variant.value<SqlStorage*>();
                    if( sqlDb )
                    {
                        if( newSqlDatabase )
                        {
                            if( newSqlDatabase->sqlDatabasePriority() < sqlDb->sqlDatabasePriority() )
                                newSqlDatabase = sqlDb;
                        }
                        else
                            newSqlDatabase = sqlDb;
                    }
                }
                d->sqlDatabase = newSqlDatabase;
                d->sqlStorageWrapper->setSqlStorage( newSqlDatabase );
            }
        }
        emit collectionRemoved( collection->collectionId() );
        QTimer::singleShot( 0, collection, SLOT( deleteLater() ) );
    }
}

void
CollectionManager::slotCollectionChanged()
{
    Amarok::Collection *collection = dynamic_cast<Amarok::Collection*>( sender() );
    if( collection )
    {
        CollectionStatus status = collectionStatus( collection->collectionId() );
        if( status & CollectionViewable )
        {
            emit collectionDataChanged( collection );
        }
    }
}

QList<Amarok::Collection*>
CollectionManager::viewableCollections() const
{
    QList<Amarok::Collection*> result;
    foreach( const CollectionPair &pair, d->collections )
    {
        if( pair.second & CollectionViewable )
        {
            result << pair.first;
        }
    }
    return result;
}

QList<Amarok::Collection*>
CollectionManager::queryableCollections() const
{
    QList<Amarok::Collection*> result;
    foreach( const CollectionPair &pair, d->collections )
        if( pair.second & CollectionQueryable )
            result << pair.first;
    return result;

}

Amarok::Collection*
CollectionManager::primaryCollection() const
{
    return d->primaryCollection;
}

SqlStorage*
CollectionManager::sqlStorage() const
{
    return d->sqlStorageWrapper;
}

Meta::TrackList
CollectionManager::tracksForUrls( const KUrl::List &urls )
{
    DEBUG_BLOCK

    debug() << "adding " << urls.size() << " tracks";

    Meta::TrackList tracks;
    foreach( const KUrl &url, urls )
    {
        Meta::TrackPtr track = trackForUrl( url );
        if( track )
            tracks.append( track );
    }
    return tracks;
}

Meta::TrackPtr
CollectionManager::trackForUrl( const KUrl &url )
{
    //TODO method stub
    //check all collections
    //might be a podcast, in that case we'll have additional meta information
    //might be a lastfm track, another stream
    //or a file which is not in any collection
    if( !url.isValid() )
        return Meta::TrackPtr( 0 );

    foreach( Amarok::TrackProvider *provider, d->trackProviders )
    {
        if( provider->possiblyContainsTrack( url ) )
        {
            Meta::TrackPtr track = provider->trackForUrl( url );
            if( track )
                return track;
        }
    }

    if( url.protocol() == "http" || url.protocol() == "mms" || url.protocol() == "smb" )
        return Meta::TrackPtr( new MetaStream::Track( url ) );

    if( url.protocol() == "file" && EngineController::canDecode( url ) )       
        return Meta::TrackPtr( new MetaFile::Track( url ) );

    return Meta::TrackPtr( 0 );
}

void
CollectionManager::relatedArtists( Meta::ArtistPtr artist, int maxArtists )
{
    if( !artist )
        return;

    m_maxArtists = maxArtists;
    SqlStorage *sql = sqlStorage();
    QString query = QString( "SELECT suggestion FROM related_artists WHERE artist = '%1' ORDER BY %2 LIMIT %3 OFFSET 0;" )
               .arg( sql->escape( artist->name() ), sql->randomFunc(), QString::number( maxArtists ) );

    QStringList artistNames = sql->query( query );
    //TODO: figure out a way to retrieve similar artists from last.fm here
    /*if( artistNames.isEmpty() )
    {
        artistNames = Scrobbler::instance()->similarArtists( Qt::escape( artist->name() ) );
    }*/
    QueryMaker *qm = queryMaker();
    foreach( const QString &artistName, artistNames )
    {
        qm->addFilter( Meta::valArtist, artistName, true, true );
    }
    qm->setQueryType( QueryMaker::Artist );
    qm->limitMaxResultSize( maxArtists );

    connect( qm, SIGNAL( newResultReady( QString, Meta::ArtistList ) ),
             this, SLOT( slotArtistQueryResult( QString, Meta::ArtistList ) ) );

    connect( qm, SIGNAL( queryDone() ), this, SLOT( slotContinueRelatedArtists() ) );

    m_resultEmitted = false;
    m_resultArtistList.clear();
    m_artistNameSet.clear();

    qm->run();
}

void
CollectionManager::slotArtistQueryResult( QString collectionId, Meta::ArtistList artists )
{
    Q_UNUSED(collectionId);

    foreach( Meta::ArtistPtr artist, artists )
    {
        if( !m_artistNameSet.contains( artist->name() ) )
        {
            m_resultArtistList.append( artist );
            m_artistNameSet.insert( artist->name() );
            if( m_resultArtistList.size() == m_maxArtists )
            {
                m_resultEmitted = true;
                emit( foundRelatedArtists( m_resultArtistList ) );
                return;
            }
        }
    }
    if( m_resultArtistList.size() == m_maxArtists && !m_resultEmitted )
    {
        m_resultEmitted = true;
        emit( foundRelatedArtists( m_resultArtistList ) );
    }
}

void
CollectionManager::slotContinueRelatedArtists() //SLOT
{
    disconnect( this, SLOT( slotArtistQueryResult( QString, Meta::ArtistList ) ) );

    disconnect( this, SLOT( slotContinueRelatedArtists() ) );

    if( !m_resultEmitted )
    {
        m_resultEmitted = true;
        emit( foundRelatedArtists( m_resultArtistList ) );
    }
    QObject *s = sender();
    if( s )
        s->deleteLater();
}

void
CollectionManager::addUnmanagedCollection( Amarok::Collection *newCollection, CollectionStatus defaultStatus )
{
    if( newCollection && d->unmanagedCollections.indexOf( newCollection ) == -1 )
    {
        const QMetaObject *mo = metaObject();
        const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "CollectionStatus" ) );
        const QString &value = KGlobal::config()->group( "CollectionManager" ).readEntry( newCollection->collectionId() );
        int enumValue = me.keyToValue( value.toLocal8Bit().constData() );
        CollectionStatus status;
        enumValue == -1 ? status = defaultStatus : status = (CollectionStatus) enumValue;
        d->unmanagedCollections.append( newCollection );
        CollectionPair pair( newCollection, status );
        d->collections.append( pair );
        d->trackProviders.append( newCollection );
        if( status & CollectionViewable )
        {
            emit collectionAdded( newCollection );
            emit collectionAdded( newCollection, status );
        }
        emit trackProviderAdded( newCollection );
    }
}

void
CollectionManager::removeUnmanagedCollection( Amarok::Collection *collection )
{
    //do not remove it from collection if it is not in unmanagedCollections
    if( collection && d->unmanagedCollections.removeAll( collection ) )
    {
        CollectionPair pair( collection, collectionStatus( collection->collectionId() ) );
        d->collections.removeAll( pair );
        d->trackProviders.removeAll( collection );
        emit collectionRemoved( collection->collectionId() );
    }
}

void
CollectionManager::setCollectionStatus( const QString &collectionId, CollectionStatus status )
{
    foreach( const CollectionPair &pair, d->collections )
    {
        if( pair.first->collectionId() == collectionId )
        {
            if( ( pair.second & CollectionViewable ) &&
               !( status & CollectionViewable ) )
            {
                emit collectionRemoved( collectionId );
            }
            else if( ( pair.second & CollectionQueryable ) &&
                    !( status & CollectionViewable ) )
            {
                emit collectionAdded( pair.first );
                emit collectionAdded( pair.first, pair.second );
            }
            CollectionPair &pair2 = const_cast<CollectionPair&>( pair );
            pair2.second = status;
            const QMetaObject *mo = metaObject();
            const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "CollectionStatus" ) );
            KGlobal::config()->group( "CollectionManager" ).writeEntry( collectionId, me.valueToKey( status ) );
            break;
        }
    }
}

CollectionManager::CollectionStatus
CollectionManager::collectionStatus( const QString &collectionId ) const
{
    foreach( const CollectionPair &pair, d->collections )
    {
        if( pair.first->collectionId() == collectionId )
        {
            return pair.second;
        }
    }
    return CollectionDisabled;
}

QHash<Amarok::Collection*, CollectionManager::CollectionStatus>
CollectionManager::collections() const
{
    QHash<Amarok::Collection*, CollectionManager::CollectionStatus> result;
    foreach( const CollectionPair &pair, d->collections )
    {
        result.insert( pair.first, pair.second );
    }
    return result;
}

void
CollectionManager::addTrackProvider( Amarok::TrackProvider *provider )
{
    d->trackProviders.append( provider );
    emit trackProviderAdded( provider );
}

void
CollectionManager::removeTrackProvider( Amarok::TrackProvider *provider )
{
    d->trackProviders.removeAll( provider );
}

#include "CollectionManager.moc"

