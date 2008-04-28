/*
 *  Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#include "CollectionLocation.h"
#include "Collection.h"
#include "QueryMaker.h"

CollectionLocation::CollectionLocation()
    :QObject()
    , m_destination( 0 )
    , m_sourceTracks()
    , m_removeSources( false )
    , m_parentCollection( 0 )
{
    //nothing to do
}

CollectionLocation::CollectionLocation( const Collection* parentCollection)
    :QObject()
    , m_destination( 0 )
    , m_sourceTracks()
    , m_removeSources( false )
    , m_parentCollection( parentCollection )
{
    //nothing to do
}

CollectionLocation::~CollectionLocation()
{
    //nothing to do
}

const Collection*
CollectionLocation::collection() const
{
    return m_parentCollection;
}

QString
CollectionLocation::prettyLocation() const
{
    return QString();
}

bool
CollectionLocation::isWriteable() const
{
    return false;
}

bool
CollectionLocation::isOrganizable() const
{
    return false;
}

void
CollectionLocation::prepareCopy( Meta::TrackPtr track, CollectionLocation *destination )
{
    Meta::TrackList list;
    list.append( track );
    prepareCopy( list, destination );
}


void
CollectionLocation::prepareCopy( const Meta::TrackList &tracks, CollectionLocation *destination )
{
    if( !destination->isWriteable() )
    {
        destination->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    startWorkflow( tracks, false );
}

void
CollectionLocation::prepareCopy( QueryMaker *qm, CollectionLocation *destination )
{
    if( !destination->isWriteable() )
    {
        destination->deleteLater();
        qm->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    m_removeSources = false;
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( resultReady( QString, Meta::TrackList ) ) );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    qm->startTrackQuery();
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
    if( !destination->isWriteable() )
    {
        destination->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    startWorkflow( tracks, true );
}

void
CollectionLocation::prepareMove( QueryMaker *qm, CollectionLocation *destination )
{
    if( !destination->isWriteable() )
    {
        destination->deleteLater();
        qm->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    m_removeSources = true;
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), SLOT( resultReady( QString, Meta::TrackList ) ) );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ) );
    qm->startTrackQuery();
    qm->run();
}

bool
CollectionLocation::remove( Meta::TrackPtr track )
{
    Q_UNUSED( track )
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
    QMap<Meta::TrackPtr, KUrl> urls;
    foreach( Meta::TrackPtr track, tracks )
    {
        if( track->isPlayable() )
            urls.insert( track, track->playableUrl() );
    }

    slotGetKIOCopyableUrlsDone( urls );
}

void
CollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    //reimplement in implementations which are writeable
    Q_UNUSED( sources )
    slotCopyOperationFinished();
}

void
CollectionLocation::showSourceDialog( const Meta::TrackList &tracks )
{
    Q_UNUSED( tracks )
    slotShowSourceDialogDone();
}

void
CollectionLocation::showDestinationDialog( const Meta::TrackList &tracks, bool removeSources )
{
    Q_UNUSED( tracks )
    Q_UNUSED( removeSources )
    slotShowDestinationDialogDone();
}

void
CollectionLocation::slotGetKIOCopyableUrlsDone( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    emit startCopy( sources, m_removeSources );
}

void
CollectionLocation::slotCopyOperationFinished()
{
    emit finishCopy( m_removeSources );
}

void
CollectionLocation::slotShowSourceDialogDone()
{
    emit prepareOperation( m_sourceTracks, m_removeSources );
}

void
CollectionLocation::slotShowDestinationDialogDone()
{
    emit operationPrepared();
}

void
CollectionLocation::slotPrepareOperation( const Meta::TrackList &tracks, bool removeSources )
{
    showDestinationDialog( tracks, removeSources );
}

void
CollectionLocation::slotOperationPrepared()
{
    getKIOCopyableUrls( m_sourceTracks );
}

void
CollectionLocation::slotStartCopy( const QMap<Meta::TrackPtr, KUrl> &sources, bool removeSources )
{
    m_removeSources = removeSources;
    copyUrlsToCollection( sources );
}

void
CollectionLocation::slotFinishCopy( bool removeSources )
{
    if( removeSources )
        removeSourceTracks( m_sourceTracks );
    m_sourceTracks.clear();
    m_destination->deleteLater();
    m_destination = 0;
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
    Q_UNUSED( collectionId )
    m_sourceTracks << tracks;
}

void
CollectionLocation::queryDone()
{
    QObject *obj = sender();
    if( obj )
    {
        obj->deleteLater();
    }
    if( m_removeSources )
    {
        prepareMove( m_sourceTracks, m_destination );
    }
    else
    {
        prepareCopy( m_sourceTracks, m_destination );
    }
    m_removeSources = false;    //do not remove the sources because of a bug
}

void
CollectionLocation::setupConnections()
{
    connect( this, SIGNAL( prepareOperation( Meta::TrackList, bool ) ),
             m_destination, SLOT( slotPrepareOperation( Meta::TrackList, bool ) ) );
    connect( m_destination, SIGNAL( operationPrepared() ), SLOT( slotOperationPrepared() ) );
    connect( this, SIGNAL( startCopy( QMap<Meta::TrackPtr, KUrl>, bool ) ),
             m_destination, SLOT( slotStartCopy( QMap<Meta::TrackPtr, KUrl>, bool ) ) );
    connect( m_destination, SIGNAL( finishCopy( bool ) ),
             this, SLOT( slotFinishCopy( bool ) ) );
    connect( this, SIGNAL( aborted() ), SLOT( slotAborted() ) );
    connect( m_destination, SIGNAL( aborted() ), SLOT( slotAborted() ) );
}

void
CollectionLocation::startWorkflow( const Meta::TrackList &tracks, bool removeSources )
{
    m_removeSources = removeSources;
    m_sourceTracks = tracks;
    setupConnections();
    showSourceDialog( tracks );
}

void
CollectionLocation::removeSourceTracks( const Meta::TrackList &tracks )
{
    Meta::TrackList notDeletableTracks;
    foreach( Meta::TrackPtr track, tracks )
    {
        if( !remove( track ) )
            notDeletableTracks.append( track );
    }
    //TODO inform user about tracks which were not deleted
}

#include "CollectionLocation.moc"
