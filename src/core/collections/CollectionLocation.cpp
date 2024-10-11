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

#include "CollectionLocation.h"

#include "core/capabilities/TranscodeCapability.h"
#include "core/collections/Collection.h"
#include "core/collections/CollectionLocationDelegate.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/transcoding/TranscodingConfiguration.h"
#include "core/transcoding/TranscodingController.h"

#include <KLocalizedString>
#include <QDir>
#include <QTimer>

using namespace Collections;

CollectionLocation::CollectionLocation()
    :QObject()
    , m_destination( nullptr )
    , m_source( nullptr )
    , m_sourceTracks()
    , m_parentCollection( nullptr )
    , m_removeSources( false )
    , m_isRemoveAction( false )
    , m_noRemoveConfirmation( false )
    , m_transcodingConfiguration( Transcoding::JUST_COPY )
{
    //nothing to do
}

CollectionLocation::CollectionLocation( Collections::Collection *parentCollection)
    :QObject()
    , m_destination( nullptr )
    , m_source( nullptr )
    , m_sourceTracks()
    , m_parentCollection( parentCollection )
    , m_removeSources( false )
    , m_isRemoveAction( false )
    , m_noRemoveConfirmation( false )
    , m_transcodingConfiguration( Transcoding::JUST_COPY )
{
    //nothing to do
}

CollectionLocation::~CollectionLocation()
{
    //nothing to do
}

Collections::Collection*
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
CollectionLocation::prepareCopy( const Meta::TrackPtr &track, CollectionLocation *destination )
{
    Q_ASSERT(destination);
    Meta::TrackList list;
    list.append( track );
    prepareCopy( list, destination );
}

void
CollectionLocation::prepareCopy( const Meta::TrackList &tracks, CollectionLocation *destination )
{
    if( !destination->isWritable() )
    {
        CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        destination->deleteLater();
        deleteLater();
        return;
    }

    m_destination = destination;
    m_destination->setSource( this );
    startWorkflow( tracks, false );
}

void
CollectionLocation::prepareCopy( Collections::QueryMaker *qm, CollectionLocation *destination )
{
    DEBUG_BLOCK
    if( !destination->isWritable() )
    {
        CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        destination->deleteLater();
        qm->deleteLater();
        deleteLater();
        return;
    }
    m_destination = destination;
    m_removeSources = false;
    m_isRemoveAction = false;
    connect( qm, &Collections::QueryMaker::newTracksReady, this, &CollectionLocation::resultReady );
    connect( qm, &Collections::QueryMaker::queryDone, this, &CollectionLocation::queryDone );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->run();
}

void
CollectionLocation::prepareMove( const Meta::TrackPtr &track, CollectionLocation *destination )
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
        CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        destination->deleteLater();
        deleteLater();
        return;
    }

    m_destination = destination;
    m_destination->setSource( this );
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
    connect( qm, &Collections::QueryMaker::newTracksReady, this, &CollectionLocation::resultReady );
    connect( qm, &Collections::QueryMaker::queryDone, this, &CollectionLocation::queryDone );
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

    connect( qm, &Collections::QueryMaker::newTracksReady, this, &CollectionLocation::resultReady );
    connect( qm, &Collections::QueryMaker::queryDone, this, &CollectionLocation::queryDone );
    qm->setQueryType( Collections::QueryMaker::Track );
    qm->run();
}

bool
CollectionLocation::insert( const Meta::TrackPtr &track, const QString &url )
{
    Q_UNUSED( track )
    Q_UNUSED( url )
    warning() << __PRETTY_FUNCTION__ << "Don't call this method. It exists only because"
              << "database importers need it. Call prepareCopy() instead.";
    return false;
}

void
CollectionLocation::abort()
{
    Q_EMIT aborted();
}

void
CollectionLocation::getKIOCopyableUrls( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    QMap<Meta::TrackPtr, QUrl> urls;
    for( Meta::TrackPtr track : tracks )
    {
        if( track->isPlayable() )
        {
            urls.insert( track, track->playableUrl() );
            debug() << "adding url " << track->playableUrl();
        }
    }

    slotGetKIOCopyableUrlsDone( urls );
}

void
CollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                          const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    //reimplement in implementations which are writable
    Q_UNUSED( sources )
    Q_UNUSED( configuration )
    slotCopyOperationFinished();
}

void
CollectionLocation::removeUrlsFromCollection( const Meta::TrackList &sources )
{
    DEBUG_BLOCK
    //reimplement in implementations which are writable
    Q_UNUSED( sources )
    slotRemoveOperationFinished();
}

void
CollectionLocation::showSourceDialog( const Meta::TrackList &tracks, bool removeSources )
{
    Q_UNUSED( tracks )
    Q_UNUSED( removeSources )

    m_transcodingConfiguration = getDestinationTranscodingConfig();
    if( m_transcodingConfiguration.isValid() )
        slotShowSourceDialogDone();
    else
        abort();
}

Transcoding::Configuration
CollectionLocation::getDestinationTranscodingConfig()
{
    Transcoding::Configuration configuration( Transcoding::JUST_COPY );
    Collection *destCollection = destination() ? destination()->collection() : nullptr;
    if( !destCollection )
        return configuration;
    if( !destCollection->has<Capabilities::TranscodeCapability>() )
        return configuration;
    QScopedPointer<Capabilities::TranscodeCapability> tc(
        destCollection->create<Capabilities::TranscodeCapability>() );
    if( !tc )
        return configuration;

    Transcoding::Controller* tcC = Amarok::Components::transcodingController();
    QSet<Transcoding::Encoder> availableEncoders;
    if( tcC )
        availableEncoders = tcC->availableEncoders();

    Transcoding::Configuration saved = tc->savedConfiguration();
    if( saved.isValid() && ( saved.isJustCopy() || availableEncoders.contains( saved.encoder() ) ) )
        return saved;
    // saved configuration was not available or was invalid, ask user

    CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
    bool saveConfiguration = false;
    CollectionLocationDelegate::OperationType operation = CollectionLocationDelegate::Copy;
    if( isGoingToRemoveSources() )
        operation = CollectionLocationDelegate::Move;
    if( collection() && collection() == destination()->collection() )
        operation = CollectionLocationDelegate::Move; // organizing
    configuration = delegate->transcode( tc->playableFileTypes(), &saveConfiguration,
                                         operation, destCollection->prettyName(),
                                         saved );
    if( configuration.isValid() )
    {
        if( saveConfiguration )
            tc->setSavedConfiguration( configuration );
        else //save the trackSelection value for restore anyway
            tc->setSavedConfiguration( Transcoding::Configuration( Transcoding::INVALID,
                                        configuration.trackSelection() ) );
    }
    return configuration; // may be invalid, it means user has hit cancel
}

void
CollectionLocation::showDestinationDialog( const Meta::TrackList &tracks,
                                           bool removeSources,
                                           const Transcoding::Configuration &configuration )
{
    Q_UNUSED( tracks )
    Q_UNUSED( configuration )
    setGoingToRemoveSources( removeSources );
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

QString
CollectionLocation::operationText( const Transcoding::Configuration &configuration )
{
    if( source()->collection() == collection() )
    {
        if( configuration.isJustCopy() )
            return i18n( "Organize tracks" );
        else
            return i18n( "Transcode and organize tracks" );
    }
    if( isGoingToRemoveSources() )
    {
        if( configuration.isJustCopy() )
            return i18n( "Move tracks" );
        else
            return i18n( "Transcode and move tracks" );
    }
    else
    {
        if( configuration.isJustCopy() )
            return i18n( "Copy tracks" );
        else
            return i18n( "Transcode and copy tracks" );
    }
}

QString
CollectionLocation::operationInProgressText( const Transcoding::Configuration &configuration,
                                                int trackCount, QString destinationName )
{
    if( destinationName.isEmpty() )
        destinationName = prettyLocation();
    if( source()->collection() == collection() )
    {
        if( configuration.isJustCopy() )
            return i18np( "Organizing one track",
                          "Organizing %1 tracks", trackCount );
        else
            return i18np( "Transcoding and organizing one track",
                          "Transcoding and organizing %1 tracks", trackCount );
    }
    if( isGoingToRemoveSources() )
    {
        if( configuration.isJustCopy() )
            return i18np( "Moving one track to %2",
                          "Moving %1 tracks to %2", trackCount, destinationName );
        else
            return i18np( "Transcoding and moving one track to %2",
                          "Transcoding and moving %1 tracks to %2", trackCount, destinationName );
    }
    else
    {
        if( configuration.isJustCopy() )
            return i18np( "Copying one track to %2",
                          "Copying %1 tracks to %2", trackCount, destinationName );
        else
            return i18np( "Transcoding and copying one track to %2",
                          "Transcoding and copying %1 tracks to %2", trackCount, destinationName );
    }
}

void
CollectionLocation::slotGetKIOCopyableUrlsDone( const QMap<Meta::TrackPtr, QUrl> &sources )
{
    Q_EMIT startCopy( sources, m_transcodingConfiguration );
}

void
CollectionLocation::slotCopyOperationFinished()
{
    Q_EMIT finishCopy();
}

void
CollectionLocation::slotRemoveOperationFinished()
{
    Q_EMIT finishRemove();
}

void
CollectionLocation::slotShowSourceDialogDone()
{
    Q_EMIT prepareOperation( m_sourceTracks, m_removeSources, m_transcodingConfiguration );
}

void
CollectionLocation::slotShowDestinationDialogDone()
{
    Q_EMIT operationPrepared();
}

void
CollectionLocation::slotShowRemoveDialogDone()
{
    Q_EMIT startRemove();
}

void
CollectionLocation::slotShowSourceDialog()
{
    showSourceDialog( m_sourceTracks, m_removeSources );
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
CollectionLocation::slotStartCopy( const QMap<Meta::TrackPtr, QUrl> &sources,
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
        m_destination = nullptr;
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

    Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
    if( m_tracksWithError.size() > 0 )
    {
        delegate->errorDeleting( this, m_tracksWithError.keys() );
        m_tracksWithError.clear();
    }

    QStringList dirsToRemove;
    debug() << "remove finished updating";
    for( Meta::TrackPtr track : m_tracksSuccessfullyTransferred )
    {
        if(!track)
            continue;

        if( track->playableUrl().isLocalFile() )
            dirsToRemove.append( track->playableUrl().adjusted( QUrl::RemoveFilename ).path() );
    }

    if( !dirsToRemove.isEmpty() && delegate->deleteEmptyDirs( this ) )
    {
        debug() << "Removing empty directories";
        dirsToRemove.removeDuplicates();
        dirsToRemove.sort();
        while( !dirsToRemove.isEmpty() )
        {
            QDir dir( dirsToRemove.takeLast() );
            if( !dir.exists() )
                continue;

            dir.setFilter( QDir::NoDotAndDotDot );
            while( !dir.isRoot() && dir.isEmpty() )
            {
                const QString name = dir.dirName();
                dir.cdUp();
                if( !dir.rmdir( name ) )
                {
                    debug() << "Unable to remove " << name;
                    break;
                }
            }
        }
    }

    m_tracksSuccessfullyTransferred.clear();
    m_sourceTracks.clear();
    this->deleteLater();
}

void
CollectionLocation::slotAborted()
{
    if( m_destination )
        m_destination->deleteLater();
    deleteLater();
}

void
CollectionLocation::resultReady( const Meta::TrackList &tracks )
{
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
        prepareCopy( m_sourceTracks, m_destination );
    }
}

void
CollectionLocation::setupConnections()
{
    connect( this, &CollectionLocation::prepareOperation,
             m_destination, &Collections::CollectionLocation::slotPrepareOperation );
    connect( m_destination, &Collections::CollectionLocation::operationPrepared,
             this, &CollectionLocation::slotOperationPrepared );
    connect( this, &CollectionLocation::startCopy,
             m_destination, &Collections::CollectionLocation::slotStartCopy );
    connect( m_destination, &Collections::CollectionLocation::finishCopy,
             this, &CollectionLocation::slotFinishCopy );
    connect( this, &CollectionLocation::aborted, this, &CollectionLocation::slotAborted );
    connect( m_destination, &Collections::CollectionLocation::aborted, this, &CollectionLocation::slotAborted );
}

void
CollectionLocation::setupRemoveConnections()
{
    connect( this, &CollectionLocation::aborted,
             this, &CollectionLocation::slotAborted );
    connect( this, &CollectionLocation::startRemove,
             this, &CollectionLocation::slotStartRemove );
    connect( this, &CollectionLocation::finishRemove,
             this, &CollectionLocation::slotFinishRemove );
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
        // show dialog in next mainloop iteration so that prepare[Something]() returns quickly
        QTimer::singleShot( 0, this, &CollectionLocation::slotShowSourceDialog );
}

void
CollectionLocation::startRemoveWorkflow( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    m_sourceTracks = tracks;
    setupRemoveConnections();
    if( tracks.isEmpty() )
        abort();
    else
        showRemoveDialog( tracks );
}

void
CollectionLocation::removeSourceTracks( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    debug() << "Transfer errors:" << m_tracksWithError.count() << "of" << tracks.count();

    for( Meta::TrackPtr track : m_tracksWithError.keys() )
    {
        debug() << "transfer error for track" << track->playableUrl();
    }

    QSet<Meta::TrackPtr> toRemove(tracks.begin(), tracks.end());
    QList<Meta::TrackPtr> trackswitherrorkeys=m_tracksWithError.keys();
    QSet<Meta::TrackPtr> errored(trackswitherrorkeys.begin(), trackswitherrorkeys.end());
    toRemove.subtract( errored );

    // start the remove workflow
    setHidingRemoveConfirm( true );
    prepareRemove( toRemove.values() );
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

