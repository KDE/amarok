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
    /* TODO: implement using MemoryCollection access.
    debug() << track->playableUrl().url();
    MetaFile::TrackPtr fileTrack = MetaFile::TrackPtr::dynamicCast( track );
    if( fileTrack.isNull() )
    {
        error() << "TrackPtr passed was not a MetaFile::TrackPtr";
        return false;
    }

    KUrl filePath = fileTrack->playableUrl();
    if( !m_umsCollection->musicPath().isParentOf( filePath ) )
    {
        error() << "This track is not in the music path of this UMS collection";
        return false;
    }
    */

    return false;
}

bool
UmsCollectionLocation::insert( const Meta::TrackPtr &track, const QString &url )
{
    Q_UNUSED( track )
    Q_UNUSED( url )
    //TODO: implement if really required. UMS does not have a database.
    return false;
}

void
UmsCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources,
                                             const Transcoding::Configuration &configuration )
{
    //TODO: transcoding options later
    Q_UNUSED( configuration )

    //TODO: disable scanning until we are done with copying

    QString loggerText = i18np( "Copying one track to %2", "Copying %1 tracks to %2",
                                 sources.count(), m_umsCollection->prettyName() );
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
            return;
        }
        transferJob->addCopy( i.value(), destination );
    }
    connect( transferJob, SIGNAL(fileTransferDone( KUrl )), m_umsCollection,
             SLOT(slotTrackAdded( KUrl )) );
    //TODO: make cancelable.
    Amarok::Components::logger()->newProgressOperation( transferJob, loggerText, transferJob,
                                                        SLOT(slotCancel()) );

    connect( transferJob, SIGNAL(finished( KJob * )), SLOT(slotCopyOperationFinished()) );
    connect( transferJob, SIGNAL(finished( KJob * )), m_umsCollection,
             SLOT(slotConnectionUpdated()) );

    transferJob->start();
}

void
UmsCollectionLocation::removeUrlsFromCollection( const Meta::TrackList &sources )
{
    KUrl::List sourceUrls;
    foreach( const Meta::TrackPtr track, sources )
        sourceUrls << track->playableUrl();

    QString loggerText = i18np( "Removing one track from %2",
                                "Removing %1 tracks from %2", sourceUrls.count(),
                                m_umsCollection->prettyName() );
    KIO::DeleteJob *delJob = KIO::del( sourceUrls, KIO::HideProgressInfo );
    //TODO: make cancelable.
    Amarok::Components::logger()->newProgressOperation( delJob, loggerText );

    connect( delJob, SIGNAL(finished( KJob * )), SLOT(slotRemoveOperationFinished()) );
}

UmsTransferJob::UmsTransferJob( UmsCollectionLocation *location )
    : KCompositeJob( location )
    , m_location( location )
    , m_cancled( false )
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

    startNextJob();
}

void
UmsTransferJob::slotCancel()
{
    DEBUG_BLOCK
}

void
UmsTransferJob::startNextJob()
{
    if( m_transferList.isEmpty() )
    {
        emitResult();
        return;
    }

    KUrlPair urlPair = m_transferList.takeFirst();
    //TODO: add move as well.
    KIO::FileCopyJob *job = KIO::file_copy( urlPair.first, urlPair.second, -1,
                                            KIO::HideProgressInfo );
    connect( job, SIGNAL(percent( KJob *, unsigned long )),
             SLOT(slotChildJobPercent( KJob *, unsigned long )) );
    QString loggerText = i18np( "Copying one track to %2", "Copying %1 tracks to %2",
                                m_transferList.count(),
                                m_location->collection()->prettyName() );
    emit infoMessage( this, loggerText, loggerText );
    addSubjob( job );
}

void
UmsTransferJob::slotChildJobPercent( KJob *job, unsigned long percentage )
{
    Q_UNUSED(job)
    emit percent( this, percentage );
}

void
UmsTransferJob::slotResult( KJob *job )
{
    removeSubjob( job );

    if( job->error() == KJob::NoError )
    {
        KIO::FileCopyJob *copyJob = dynamic_cast<KIO::FileCopyJob *>( job );
        if( copyJob )
            emit fileTransferDone( copyJob->destUrl() );
    }
    startNextJob();
}
