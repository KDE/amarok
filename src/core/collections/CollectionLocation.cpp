/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#define DEBUG_PREFIX "CollectionLocation"

#include "core/collections/CollectionLocation.h"

#include "core/collections/Collection.h"
#include "core/collections/CollectionLocationDelegate.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/collections/QueryMaker.h"
#include "core/capabilities/UpdateCapability.h"

using namespace Collections;

CollectionLocation::CollectionLocation()
    :QObject()
    , m_destination( 0 )
    , m_source( 0 )
    , m_sourceTracks()
    , m_parentCollection( 0 )
    , m_removeSources( false )
    , m_isRemoveAction( false )
    , m_noRemoveConfirmation( false )
    , m_transcodingConfiguration( Transcoding::Configuration() )
{
    //nothing to do
}

CollectionLocation::CollectionLocation( const Collections::Collection* parentCollection)
    :QObject()
    , m_destination( 0 )
    , m_source( 0 )
    , m_sourceTracks()
    , m_parentCollection( parentCollection )
    , m_removeSources( false )
    , m_isRemoveAction( false )
    , m_noRemoveConfirmation( false )
    , m_transcodingConfiguration( Transcoding::Configuration() )
{
    //nothing to do
}

CollectionLocation::~CollectionLocation()
{
    //nothing to do
}

const Collections::Collection*
CollectionLocation::collection() const
{
    return m_parentCollection;
}

QString
CollectionLocation::prettyLocation() const
{
    return QString();
}

QStringList
CollectionLocation::actualLocation() const
{
    return QStringList();
}

bool
CollectionLocation::isWritable() const
{
    return false;
}

bool
CollectionLocation::isOrganizable() const
{
    return false;
}

void
CollectionLocation::prepareCopy( Meta::TrackPtr track, CollectionLocation *destination,
                                 const Transcoding::Configuration &configuration )
{
    debug() << "prepare copy 1 track from"<<collection()->collectionId()<<"to"<<destination->collection()->collectionId();
    Meta::TrackList list;
    list.append( track );
    prepareCopy( list, destination, configuration );
}


void
CollectionLocation::prepareCopy( const Meta::TrackList &tracks, CollectionLocation *destination,
                                 const Transcoding::Configuration &configuration )
{
    if( !destination->isWritable() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        destination->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    m_transcodingConfiguration = configuration;
    m_destination->setSource( this );
    startWorkflow( tracks, false );
}

void
CollectionLocation::prepareCopy( Collections::QueryMaker *qm, CollectionLocation *destination,
                                 const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    if( !destination->isWritable() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        destination->deleteLater();
        qm->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    m_transcodingConfiguration = configuration;
    m_removeSources = false;
    m_isRemoveAction = false;
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( resultReady( QString, Meta::TrackList ) ) );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->run();
}

void
CollectionLocation::prepareMove( Meta::TrackPtr track, CollectionLocation *destination )
{
    Meta::TrackList list;
    list.append( track );
    prepareMove( list, destination );
}

void
CollectionLocation::prepareMove( const Meta::TrackList &tracks, CollectionLocation *destination )
{
    DEBUG_BLOCK
    if( !destination->isWritable() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        destination->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    destination->setSource( this );
    startWorkflow( tracks, true );
}

void
CollectionLocation::prepareMove( Collections::QueryMaker *qm, CollectionLocation *destination )
{
    DEBUG_BLOCK
    if( !destination->isWritable() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        destination->deleteLater();
        qm->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    m_isRemoveAction = false;
    m_removeSources = true;
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( resultReady( QString, Meta::TrackList ) ) );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->run();
}

void
CollectionLocation::prepareRemove( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    if( !isWritable() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        deleteLater();
        return;
    }
    startRemoveWorkflow( tracks );
}

void
CollectionLocation::prepareRemove( Collections::QueryMaker *qm )
{
    DEBUG_BLOCK
    if( !isWritable() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        qm->deleteLater();
        deleteLater();
        return;
    }

    m_isRemoveAction = true;
    m_removeSources = false;

    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( resultReady( QString, Meta::TrackList ) ) );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->run();
}

bool
CollectionLocation::remove( const Meta::TrackPtr &track )
{
    Q_UNUSED( track )
    return false;
}

bool
CollectionLocation::insert( const Meta::TrackPtr &track, const QString &url )
{
    Q_UNUSED( track )
    Q_UNUSED( url )
    return false;
}


void
CollectionLocation::abort()
{
    emit aborted();
}

void
CollectionLocation::getKIOCopyableUrls( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    QMap<Meta::TrackPtr, KUrl> urls;
    foreach( Meta::TrackPtr track, tracks )
    {
        if( track->isPlayable() )
        {
            urls.insert( track, track->playableUrl() );
            debug() << "adding url " << track->playableUrl();
        }
    }
    debug() << "Format is " << m_transcodingConfiguration.encoder();

    slotGetKIOCopyableUrlsDone( urls );
}

void
CollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources,
                                          const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    //reimplement in implementations which are writeable
    Q_UNUSED( sources )
    Q_UNUSED( configuration )
    slotCopyOperationFinished();
}

void
CollectionLocation::removeUrlsFromCollection( const Meta::TrackList &sources )
{
    DEBUG_BLOCK
    //reimplement in implementations which are writeable
    Q_UNUSED( sources )
    slotRemoveOperationFinished();
}

void
CollectionLocation::showSourceDialog( const Meta::TrackList &tracks, bool removeSources )
{
    Q_UNUSED( tracks )
    Q_UNUSED( removeSources )
    slotShowSourceDialogDone();
}

void
CollectionLocation::showDestinationDialog( const Meta::TrackList &tracks,
                                           bool removeSources,
                                           const Transcoding::Configuration &configuration )
{
    Q_UNUSED( tracks )
    Q_UNUSED( removeSources )
    Q_UNUSED( configuration )
    slotShowDestinationDialogDone();
}

void
CollectionLocation::showRemoveDialog( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK

    if( !isHidingRemoveConfirm() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();

        const bool del = delegate->reallyDelete( this, tracks );

        if( !del )
            slotFinishRemove();
        else
            slotShowRemoveDialogDone();
    } else
        slotShowRemoveDialogDone();
}

void
CollectionLocation::slotGetKIOCopyableUrlsDone( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    emit startCopy( sources, m_transcodingConfiguration );
}

void
CollectionLocation::slotCopyOperationFinished()
{
    emit finishCopy();
}

void
CollectionLocation::slotRemoveOperationFinished()
{
    DEBUG_BLOCK
    emit finishRemove();
}

void
CollectionLocation::slotShowSourceDialogDone()
{
    emit prepareOperation( m_sourceTracks, m_removeSources, m_transcodingConfiguration );
}

void
CollectionLocation::slotShowDestinationDialogDone()
{
    emit operationPrepared();
}

void
CollectionLocation::slotShowRemoveDialogDone()
{
    DEBUG_BLOCK
    emit startRemove();
}

void
CollectionLocation::slotPrepareOperation( const Meta::TrackList &tracks, bool removeSources,
                                          const Transcoding::Configuration &configuration )
{
    m_removeSources = removeSources;
    showDestinationDialog( tracks, removeSources, configuration );
}

void
CollectionLocation::slotOperationPrepared()
{
    getKIOCopyableUrls( m_sourceTracks );
}

void
CollectionLocation::slotStartCopy( const QMap<Meta::TrackPtr, KUrl> &sources,
                                   const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    copyUrlsToCollection( sources, configuration );
}

void
CollectionLocation::slotFinishCopy()
{
    DEBUG_BLOCK
    if( m_removeSources )
    {
        removeSourceTracks( m_tracksSuccessfullyTransferred );
        m_sourceTracks.clear();
        m_tracksSuccessfullyTransferred.clear();
    }
    else
    {
        m_sourceTracks.clear();
        m_tracksSuccessfullyTransferred.clear();

        if( m_destination )
            m_destination->deleteLater();
        m_destination = 0;
        this->deleteLater();
    }
}

void
CollectionLocation::slotStartRemove()
{
    DEBUG_BLOCK
    removeUrlsFromCollection( m_sourceTracks );
}

void
CollectionLocation::slotFinishRemove()
{
    DEBUG_BLOCK
    if( m_tracksWithError.size() > 0 )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->errorDeleting( this, m_tracksWithError.keys() );
        m_tracksWithError.clear();
    }

    debug() << "remove finished updating";
    foreach( Meta::TrackPtr track, m_tracksSuccessfullyTransferred )
    {
        if(!track)
            continue;

        QScopedPointer<Capabilities::UpdateCapability> uc( track->create<Capabilities::UpdateCapability>() );
        if(!uc)
            continue;

        uc->collectionUpdated();
    }

    m_tracksSuccessfullyTransferred.clear();
    m_sourceTracks.clear();
    this->deleteLater();
}

void
CollectionLocation::slotAborted()
{
    m_destination->deleteLater();
    deleteLater();
}

void
CollectionLocation::resultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId )
    m_sourceTracks << tracks;
}

void
CollectionLocation::queryDone()
{
    DEBUG_BLOCK
    QObject *obj = sender();
    if( obj )
    {
        obj->deleteLater();
    }
    if( m_isRemoveAction )
    {
        debug() << "we were about to remove something, lets proceed";
        prepareRemove( m_sourceTracks );
    }
    else if( m_removeSources )
    {
        debug() << "we were about to move something, lets proceed";
        prepareMove( m_sourceTracks, m_destination );
    }
    else
    {
        debug() << "we were about to copy something, lets proceed";
        prepareCopy( m_sourceTracks, m_destination, m_transcodingConfiguration );
    }
}

void
CollectionLocation::setupConnections()
{
    connect( this, SIGNAL( prepareOperation( Meta::TrackList, bool, const Transcoding::Configuration & ) ),
             m_destination, SLOT( slotPrepareOperation( Meta::TrackList, bool, const Transcoding::Configuration & ) ) );
    connect( m_destination, SIGNAL( operationPrepared() ), SLOT( slotOperationPrepared() ) );
    connect( this, SIGNAL( startCopy( QMap<Meta::TrackPtr, KUrl>, const Transcoding::Configuration & ) ),
             m_destination, SLOT( slotStartCopy( QMap<Meta::TrackPtr, KUrl>, const Transcoding::Configuration & ) ) );
    connect( m_destination, SIGNAL( finishCopy() ),
             this, SLOT( slotFinishCopy() ) );
    connect( this, SIGNAL( aborted() ), SLOT( slotAborted() ) );
    connect( m_destination, SIGNAL( aborted() ), SLOT( slotAborted() ) );
}

void
CollectionLocation::setupRemoveConnections()
{
    connect( this, SIGNAL( aborted() ), SLOT( slotAborted() ) );
    connect( this, SIGNAL( startRemove() ),
             this, SLOT( slotStartRemove() ) );
    connect( this, SIGNAL( finishRemove() ),
             this, SLOT( slotFinishRemove() ) );
}

void
CollectionLocation::startWorkflow( const Meta::TrackList &tracks, bool removeSources )
{
    DEBUG_BLOCK
    m_removeSources = removeSources;
    m_sourceTracks = tracks;
    setupConnections();
    if( tracks.size() <= 0 )
        abort();
    else
        showSourceDialog( tracks, m_removeSources );
}

void
CollectionLocation::startRemoveWorkflow( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    m_sourceTracks = tracks;
    setupRemoveConnections();
    if( tracks.size() <= 0 )
        abort(); // TODO: check if this is the right function
    else
        showRemoveDialog( tracks );
}

void
CollectionLocation::removeSourceTracks( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    debug() << "Transfer errors:" << m_tracksWithError.count() << "of" << tracks.count();

    foreach( Meta::TrackPtr track, m_tracksWithError.keys() )
    {
        debug() << "transfer error for track" << track->playableUrl();
    }

    QSet<Meta::TrackPtr> toRemove = QSet<Meta::TrackPtr>::fromList( tracks );
    QSet<Meta::TrackPtr> errored = QSet<Meta::TrackPtr>::fromList( m_tracksWithError.keys() );
    toRemove.subtract( errored );

    // start the remove workflow
    setHidingRemoveConfirm( true );
    prepareRemove( toRemove.toList() );
}

CollectionLocation*
CollectionLocation::source() const
{
    return m_source;
}

CollectionLocation*
CollectionLocation::destination() const
{
    return m_destination;
}

void
CollectionLocation::setSource( CollectionLocation *source )
{
    m_source = source;
}

void
CollectionLocation::transferSuccessful( const Meta::TrackPtr &track )
{
    m_tracksSuccessfullyTransferred.append( track );
}

bool
CollectionLocation::isGoingToRemoveSources() const
{
    return m_removeSources;
}
void
CollectionLocation::setGoingToRemoveSources( bool removeSources )
{
    m_removeSources = removeSources;
}

bool
CollectionLocation::isHidingRemoveConfirm() const
{
    return m_noRemoveConfirmation;
}

void
CollectionLocation::setHidingRemoveConfirm( bool hideRemoveConfirm )
{
    m_noRemoveConfirmation = hideRemoveConfirm;
}

void
CollectionLocation::transferError( const Meta::TrackPtr &track, const QString &error )
{
    m_tracksWithError.insert( track, error );
}

#include "CollectionLocation.moc"
