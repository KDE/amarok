/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "UmsCollectionLocation.h"

#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/interfaces/Logger.h"
#include "core-impl/meta/file/File.h"

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <kio/job.h>
#include <KUrl>
#include <kio/jobclasses.h>

UmsCollectionLocation::UmsCollectionLocation( UmsCollection *umsCollection )
    : CollectionLocation( umsCollection )
    , m_umsCollection( umsCollection )
{
}

UmsCollectionLocation::~UmsCollectionLocation()
{
}

QString
UmsCollectionLocation::prettyLocation() const
{
    return m_umsCollection->musicPath().toLocalFile( KUrl::RemoveTrailingSlash );
}

QStringList
UmsCollectionLocation::actualLocation() const
{
    return QStringList() << prettyLocation();
}

bool
UmsCollectionLocation::isWritable() const
{
    return m_umsCollection->isWritable();
}

bool
UmsCollectionLocation::isOrganizable() const
{
    return isWritable();
}

bool
UmsCollectionLocation::remove( const Meta::TrackPtr &track )
{
    Q_UNUSED( track )
    warning() << "Don't use UmsCollectionLocation::remove(). Use removeUrlsFromCollection().";
    return false;
}

bool
UmsCollectionLocation::insert( const Meta::TrackPtr &track, const QString &url )
{
    Q_UNUSED( track )
    Q_UNUSED( url )
    warning() << "Don't use UmsCollectionLocation::insert(). Use copyUrlsToCollection().";
    return false;
}

void
UmsCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources,
                                             const Transcoding::Configuration &configuration )
{
    //TODO: transcoding options later
    Q_UNUSED( configuration )

    //TODO: disable scanning until we are done with copying

    UmsTransferJob *transferJob = new UmsTransferJob( this );
    QMapIterator<Meta::TrackPtr, KUrl> i( sources );
    while( i.hasNext() )
    {
        i.next();
        KUrl destination = m_umsCollection->organizedUrl( i.key() );
        debug() << "destination is " << destination.toLocalFile();
        QDir dir( destination.directory() );
        if( !dir.exists() && !dir.mkpath( "." ) )
        {
            error() << "could not create directory to copy into.";
            abort();
        }
        m_sourceUrlToTrackMap.insert( i.value(), i.key() ); // needed for slotTrackTransferred()
        transferJob->addCopy( i.value(), destination );
    }

    connect( transferJob, SIGNAL(sourceFileTransferDone(KUrl)),
             this, SLOT(slotTrackTransferred(KUrl)) );
    connect( transferJob, SIGNAL(fileTransferDone(KUrl)),
             m_umsCollection, SLOT(slotTrackAdded(KUrl)) );
    connect( transferJob, SIGNAL(finished(KJob*)),
             this, SLOT(slotCopyOperationFinished()) );

    QString loggerText;
    if( isGoingToRemoveSources() )
        loggerText = i18np( "Moving one track to %2", "Moving %1 tracks to %2",
                                 sources.count(), m_umsCollection->prettyName() );
    else
        loggerText = i18np( "Copying one track to %2", "Copying %1 tracks to %2",
                                 sources.count(), m_umsCollection->prettyName() );
    Amarok::Components::logger()->newProgressOperation( transferJob, loggerText, transferJob,
                                                        SLOT(slotCancel()) );
    transferJob->start();
}

void
UmsCollectionLocation::removeUrlsFromCollection( const Meta::TrackList &sources )
{
    KUrl::List sourceUrls;
    foreach( const Meta::TrackPtr track, sources )
    {
        KUrl trackUrl = track->playableUrl();
        m_sourceUrlToTrackMap.insert( trackUrl, track );
        sourceUrls.append( trackUrl );
    }

    QString loggerText = i18np( "Removing one track from %2",
                                "Removing %1 tracks from %2", sourceUrls.count(),
                                m_umsCollection->prettyName() );
    KIO::DeleteJob *delJob = KIO::del( sourceUrls, KIO::HideProgressInfo );
    Amarok::Components::logger()->newProgressOperation( delJob, loggerText, delJob, SLOT(kill()) );

    // there is no deleted() signal, so assume that deleting succeeded.
    connect( delJob, SIGNAL(deleting(KIO::Job*,KUrl)), SLOT(slotTrackDeleted(KIO::Job*,KUrl)) );
    connect( delJob, SIGNAL(finished( KJob * )), SLOT(slotRemoveOperationFinished()) );
}

void
UmsCollectionLocation::slotTrackTransferred( const KUrl &sourceTrackUrl )
{
    Meta::TrackPtr sourceTrack = m_sourceUrlToTrackMap.value( sourceTrackUrl );
    if( !sourceTrack )
        warning() << __PRETTY_FUNCTION__ << ": I don't know about" << sourceTrackUrl;
    else
        // this is needed for example for "move" operation to actually remove source tracks
        source()->transferSuccessful( sourceTrack );
}

void
UmsCollectionLocation::slotTrackDeleted( KIO::Job* ,const KUrl &trackUrl )
{
    Meta::TrackPtr sourceTrack = m_sourceUrlToTrackMap.value( trackUrl );
    if( !sourceTrack )
        warning() << __PRETTY_FUNCTION__ << ": I don't know about" << trackUrl;
    else
        // this is needed to trigger CollectionLocation's functionality to remove empty dirs
        transferSuccessful( sourceTrack );
}

UmsTransferJob::UmsTransferJob( UmsCollectionLocation *location )
    : KCompositeJob( location )
    , m_location( location )
    , m_abort( false )
{
    setCapabilities( KJob::Killable );
}

void
UmsTransferJob::addCopy( const KUrl &from, const KUrl &to )
{
    m_transferList << KUrlPair( from, to );
}

void
UmsTransferJob::start()
{
    DEBUG_BLOCK;
    if( m_transferList.isEmpty() )
        return;

    m_totalTracks = m_transferList.size();
    startNextJob();
}

void
UmsTransferJob::slotCancel()
{
    m_abort = true;
}

void
UmsTransferJob::startNextJob()
{
    if( m_transferList.isEmpty() || m_abort )
    {
        emitResult();
        return;
    }

    KUrlPair urlPair = m_transferList.takeFirst();
    KIO::FileCopyJob *job = KIO::file_copy( urlPair.first, urlPair.second, -1,
                                            KIO::HideProgressInfo );
    connect( job, SIGNAL(percent( KJob *, unsigned long )),
             SLOT(slotChildJobPercent( KJob *, unsigned long )) );
    addSubjob( job );
}

void
UmsTransferJob::slotChildJobPercent( KJob *job, unsigned long percentage )
{
    Q_UNUSED(job)
    int alreadyTransferred = m_totalTracks - m_transferList.size() - 1;
    emit percent( this, ( alreadyTransferred * 100.0 + percentage ) / m_totalTracks );
}

void
UmsTransferJob::slotResult( KJob *job )
{
    removeSubjob( job );

    if( job->error() == KJob::NoError )
    {
        KIO::FileCopyJob *copyJob = dynamic_cast<KIO::FileCopyJob *>( job );
        if( copyJob )
        {
            emit sourceFileTransferDone( copyJob->srcUrl() );
            emit fileTransferDone( copyJob->destUrl() );
        }
        else
            Debug::warning() << __PRETTY_FUNCTION__ << "invalid job passed to me!";
    }
    startNextJob();
}
