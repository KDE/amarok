/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "SqlCollectionLocation.h"

#include "collectionscanner/AFTUtility.h"

#include "core/collections/CollectionLocationDelegate.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "collection/SqlStorage.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "dialogs/OrganizeCollectionDialog.h"
#include "ScanManager.h"
#include "ScanResultProcessor.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "statusbar/StatusBar.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <kdiskfreespaceinfo.h>
#include <kjob.h>
#include <KLocale>
#include <KSharedPtr>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/deletejob.h>
#include <KMessageBox>


using namespace Meta;

SqlCollectionLocation::SqlCollectionLocation( SqlCollection const *collection )
    : CollectionLocation( collection )
    , m_collection( const_cast<SqlCollection*>( collection ) )
    , m_overwriteFiles( false )
    , m_transferjob( 0 )
{
    //nothing to do
}

SqlCollectionLocation::~SqlCollectionLocation()
{
    //nothing to do
}

QString
SqlCollectionLocation::prettyLocation() const
{
    return i18n( "Local Collection" );
}

QStringList
SqlCollectionLocation::actualLocation() const
{
    return m_collection->mountPointManager()->collectionFolders();
}

bool
SqlCollectionLocation::isWritable() const
{
    DEBUG_BLOCK
    // The collection is writeable if there exists a path that has less than
    // 95% free space.
    bool path_exists_with_space = false;
    bool path_exists_writeable = false;
    QStringList folders = actualLocation();
    foreach(QString path, folders)
    {
        float used = KDiskFreeSpaceInfo::freeSpaceInfo( path ).used();
        float total = KDiskFreeSpaceInfo::freeSpaceInfo( path ).size();
	debug() << path;
	debug() << "\tused: " << used;
	debug() << "\ttotal: " << total;

        if( total <= 0 ) // protect against div by zero
            continue; //How did this happen?

        float free_space = total - used;
        debug() <<"\tfree space: " << free_space;
        if( free_space >= 500*1000*1000 ) // ~500 megabytes
            path_exists_with_space = true;

        QFileInfo info( path );
        if( info.isWritable() )
            path_exists_writeable = true;
	debug() << "\tpath_exists_writeable" << path_exists_writeable;
	debug() << "\tpath_exists_with_space" << path_exists_with_space;
    }
    return path_exists_with_space && path_exists_writeable;
}

bool
SqlCollectionLocation::isOrganizable() const
{
    return true;
}

bool
SqlCollectionLocation::remove( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    KSharedPtr<SqlTrack> sqlTrack = KSharedPtr<SqlTrack>::dynamicCast( track );
    if( sqlTrack && sqlTrack->inCollection() && sqlTrack->collection()->collectionId() == m_collection->collectionId() )
    {
        debug() << "much much";
        bool removed;
        KUrl originalUrl = m_originalUrls[track];
        // we are going to delete it from the database only if is no longer on disk
        removed = !QFile::exists( originalUrl.path() );

        if( removed )
        {
            int deviceId = m_collection->mountPointManager()->getIdForUrl( originalUrl );
            QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, originalUrl.url() );
            QString query = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" )
                                .arg( QString::number( deviceId ), m_collection->sqlStorage()->escape( rpath ) );
            QStringList res = m_collection->sqlStorage()->query( query );
            if( res.isEmpty() )
            {
                warning() << "Tried to remove a track from SqlCollection which is not in the collection";
            }
            else
            {
                int id = res[0].toInt();
                QString query = QString( "DELETE FROM tracks where url = %1;" ).arg( id );
                m_collection->sqlStorage()->query( query );
            }

            QFileInfo file( m_originalUrls[track].path() );
            QDir dir = file.dir();
            const QStringList collectionFolders = m_collection->mountPointManager()->collectionFolders();
            while( !collectionFolders.contains( dir.absolutePath() ) && !dir.isRoot() && dir.count() == 0 )
            {
                const QString name = dir.dirName();
                dir.cdUp();
                if( !dir.rmdir( name ) )
                    break;
            }

        }
        return removed;
    }
    else
    {
        debug() << "Remove Failed: track exists on disk." << m_originalUrls[track].path();
        return false;
    }
}

void
SqlCollectionLocation::showDestinationDialog( const Meta::TrackList &tracks, bool removeSources )
{
    DEBUG_BLOCK
    setGoingToRemoveSources( removeSources );

    KIO::filesize_t transferSize = 0;
    foreach( Meta::TrackPtr track, tracks )
        transferSize += track->filesize();

    QStringList actual_folders = actualLocation(); // the folders in the collection
    QStringList available_folders; // the folders which have freespace available
    foreach(QString path, actual_folders)
    {
        if( path.isEmpty() )
            continue;
        debug() << "Path" << path;
        KDiskFreeSpaceInfo spaceInfo = KDiskFreeSpaceInfo::freeSpaceInfo( path );
        if( !spaceInfo.isValid() )
            continue;

        KIO::filesize_t totalCapacity = spaceInfo.size();
        KIO::filesize_t used = spaceInfo.used();

        KIO::filesize_t freeSpace = totalCapacity - used;

        debug() << "used:" << used;
        debug() << "total:" << totalCapacity;
        debug() << "Free space" << freeSpace;
        debug() << "transfersize" << transferSize;

        if( totalCapacity <= 0 ) // protect against div by zero
            continue; //How did this happen?

        QFileInfo info( path );

        // since bad things happen when drives become totally full
	// we make sure there is at least ~500MB left
        // finally, ensure the path is writeable
        debug() << ( freeSpace - transferSize );
        if( ( freeSpace - transferSize ) > 1000*1000*500 && info.isWritable() )
            available_folders << path;
    }

    if( available_folders.size() <= 0 )
    {
        debug() << "No space available or not writable";
        CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        delegate->notWriteable( this );
        abort();
        return;
    }

    OrganizeCollectionDialog *dialog = new OrganizeCollectionDialog( tracks,
                available_folders,
                The::mainWindow(), //parent
                "", //name is unused
                true, //modal
                i18n( "Organize Files" ) //caption
            );
    connect( dialog, SIGNAL( accepted() ), SLOT( slotDialogAccepted() ) );
    connect( dialog, SIGNAL( rejected() ), SLOT( slotDialogRejected() ) );
    dialog->show();
}

void
SqlCollectionLocation::slotDialogAccepted()
{
    sender()->deleteLater();
    OrganizeCollectionDialog *dialog = qobject_cast<OrganizeCollectionDialog*>( sender() );
    m_destinations = dialog->getDestinations();
    m_overwriteFiles = dialog->overwriteDestinations();
    if( isGoingToRemoveSources() )
    {
        CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        const bool del = delegate->reallyMove( this, m_destinations.keys() );
        if( !del )
        {
            abort();
            return;
        }
    }
    slotShowDestinationDialogDone();
}

void
SqlCollectionLocation::slotDialogRejected()
{
    DEBUG_BLOCK
    sender()->deleteLater();
    abort();
}

void
SqlCollectionLocation::slotJobFinished( KJob *job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        //TODO: proper error handling
        warning() << "An error occurred when copying a file: " << job->errorString();
        source()->transferError(m_jobs.value( job ), KIO::buildErrorString( job->error(), job->errorString() ) );
        m_destinations.remove( m_jobs.value( job ) );
    }
    //we  assume that KIO works correctly...
    source()->transferSuccessful( m_jobs.value( job ) );

    m_jobs.remove( job );
    job->deleteLater();

}

void
SqlCollectionLocation::slotRemoveJobFinished( KJob *job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        //TODO: proper error handling
        warning() << "An error occurred when removing a file: " << job->errorString();
        transferError(m_removejobs.value( job ), KIO::buildErrorString( job->error(), job->errorString() ) );
    }
    else
    {
        // Remove the track from the database
        remove( m_removejobs.value( job ) );

        //we  assume that KIO works correctly...
        transferSuccessful( m_removejobs.value( job ) );
    }

    m_removejobs.remove( job );
    job->deleteLater();

    if( !startNextRemoveJob() )
    {
        m_collection->scanManager()->setBlockScan( false );
        slotRemoveOperationFinished();
    }

}

void SqlCollectionLocation::slotTransferJobFinished( KJob* job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        debug() << job->errorText();
    }
    // filter the list of destinations to only include tracks
    // that were successfully copied
    foreach( const Meta::TrackPtr &track, m_destinations.keys() )
    {
        if( !QFileInfo( m_destinations[ track ] ).exists() )
            m_destinations.remove( track );
        m_originalUrls[track] = track->playableUrl();
    }
    insertTracks( m_destinations );
    insertStatistics( m_destinations );
    m_collection->scanManager()->setBlockScan( false );
    slotCopyOperationFinished();
}

void SqlCollectionLocation::slotTransferJobAborted()
{
    DEBUG_BLOCK
    if( !m_transferjob )
        return;
    m_transferjob->kill();
    // filter the list of destinations to only include tracks
    // that were successfully copied
    foreach( const Meta::TrackPtr &track, m_destinations.keys() )
    {
        if( !QFileInfo( m_destinations[ track ] ).exists() )
            m_destinations.remove( track );
        m_originalUrls[track] = track->playableUrl();
    }
    insertTracks( m_destinations );
    insertStatistics( m_destinations );
    m_collection->scanManager()->setBlockScan( false );
    abort();
}


void
SqlCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources )
{
    m_collection->scanManager()->setBlockScan( true );  //make sure the collection scanner does not run while we are coyping stuff

    m_sources = sources;

    QString statusBarTxt;

    if( destination() == source() )
        statusBarTxt = i18n( "Organizing tracks" );
    else if ( isGoingToRemoveSources() )
        statusBarTxt = i18n( "Moving tracks" );
    else
        statusBarTxt = i18n( "Copying tracks" );

    m_transferjob = new TransferJob( this );
    The::statusBar()->newProgressOperation( m_transferjob, statusBarTxt )->setAbortSlot( this, SLOT( slotTransferJobAborted() ) );
    connect( m_transferjob, SIGNAL( result( KJob * ) ), this, SLOT( slotTransferJobFinished( KJob * ) ) );
    m_transferjob->start();
}

void
SqlCollectionLocation::removeUrlsFromCollection(  const Meta::TrackList &sources )
{
    DEBUG_BLOCK

    m_collection->scanManager()->setBlockScan( true );  //make sure the collection scanner does not run while we are deleting stuff

    m_removetracks = sources;

    if( !startNextRemoveJob() ) //this signal needs to be called no matter what, even if there are no job finishes to call it
    {
        m_collection->scanManager()->setBlockScan( false );
        slotRemoveOperationFinished();
    }
}

void
SqlCollectionLocation::insertTracks( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    QList<QVariantMap > metadata;
    QStringList urls;
    AFTUtility aftutil;
    foreach( const Meta::TrackPtr &track, trackMap.keys() )
    {
        QVariantMap trackData = Meta::Field::mapFromTrack( track );
        trackData.insert( Meta::Field::URL, trackMap[ track ] );  //store the new url of the file
        // overwrite any uidUrl that came with the track with our own sql AFT one
        trackData.insert( Meta::Field::UNIQUEID, QString( "amarok-sqltrackuid://" ) + aftutil.readUniqueId( trackMap[ track ] ) );
        metadata.append( trackData );
        urls.append( trackMap[ track ] );
    }
    ScanResultProcessor processor( m_collection );
    processor.setSqlStorage( m_collection->sqlStorage() );
    processor.setScanType( ScanResultProcessor::IncrementalScan );
    QMap<QString, uint> mtime = updatedMtime( urls );
    foreach( const QString &dir, mtime.keys() )
    {
        processor.addDirectory( dir, mtime[ dir ] );
    }
    if( !metadata.isEmpty() )
    {
        QFileInfo info( metadata.first().value( Meta::Field::URL ).toString() );
        QString currentDir = info.dir().absolutePath();
        QList<QVariantMap > currentMetadata;
        foreach( const QVariantMap &map, metadata )
        {
            debug() << "processing file " << map.value( Meta::Field::URL );
            QFileInfo info( map.value( Meta::Field::URL ).toString() );
            QString dir = info.dir().absolutePath();
            if( dir != currentDir )
            {
                processor.processDirectory( currentMetadata );
                currentDir = dir;
                currentMetadata.clear();
            }
            currentMetadata.append( map );
        }
        if( !currentMetadata.isEmpty() )
        {
            processor.processDirectory( currentMetadata );
        }
    }
    processor.commit();
}

QMap<QString, uint>
SqlCollectionLocation::updatedMtime( const QStringList &urls )
{
    QMap<QString, uint> mtime;
    foreach( const QString &url, urls )
    {
        QFileInfo fileInfo( url );
        QDir parentDir = fileInfo.dir();
        while( !mtime.contains( parentDir.absolutePath() ) && m_collection->isDirInCollection( parentDir.absolutePath() ) )
        {
            QFileInfo dir( parentDir.absolutePath() );
            mtime.insert( parentDir.absolutePath(), dir.lastModified().toTime_t() );
            if( !parentDir.cdUp() )
            {
                break;  //we arrived at the root of the filesystem
            }
        }
    }
    return mtime;
}

void
SqlCollectionLocation::insertStatistics( const QMap<Meta::TrackPtr, QString> &trackMap )
{
    SqlMountPointManager *mpm = m_collection->mountPointManager();
    foreach( const Meta::TrackPtr &track, trackMap.keys() )
    {
        QString url = trackMap[ track ];
        int deviceid = mpm->getIdForUrl( url );
        QString rpath = mpm->getRelativePath( deviceid, url );
        QString sql = QString( "SELECT COUNT(*) FROM statistics LEFT JOIN urls ON statistics.url = urls.id "
                               "WHERE urls.deviceid = %1 AND urls.rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->sqlStorage()->escape( rpath ) );
        QStringList count = m_collection->sqlStorage()->query( sql );
        if( count.isEmpty() || count.first().toInt() != 0 )    //crash if the sql is bad
        {
            continue;   //a statistics row already exists for that url, and we cannot merge the statistics
        }
        //the row will exist because this method is called after insertTracks
        QString select = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" ).arg( QString::number( deviceid ), m_collection->sqlStorage()->escape( rpath ) );
        QStringList result = m_collection->sqlStorage()->query( select );
        if( result.isEmpty() )
        {
            warning() << "SQL Query returned no results:" << select;
            continue;
        }
        QString id = result.first();    //if result is empty something is going very wrong
        //the following sql was copied from SqlMeta.cpp
        QString insert = "INSERT INTO statistics(url,rating,score,playcount,accessdate,createdate) VALUES ( %1 );";
        QString data = "%1,%2,%3,%4,%5,%6";
        data = data.arg( id, QString::number( track->rating() ), QString::number( track->score() ),
                    QString::number( track->playCount() ), QString::number( track->lastPlayed() ), QString::number( track->firstPlayed() ) );
        m_collection->sqlStorage()->insert( insert.arg( data ), "statistics" );
    }
}

bool SqlCollectionLocation::startNextJob()
{
    DEBUG_BLOCK
    if( !m_sources.isEmpty() )
    {
        Meta::TrackPtr track = m_sources.keys().first();
        KUrl src = m_sources.take( track );

        KIO::FileCopyJob *job = 0;
        KUrl dest = m_destinations[ track ];
        dest.cleanPath();

        src.cleanPath();
        debug() << "copying from " << src << " to " << dest;
        KIO::JobFlags flags = KIO::HideProgressInfo;
        if( m_overwriteFiles )
        {
            flags |= KIO::Overwrite;
        }
        QFileInfo info( dest.pathOrUrl() );
        QDir dir = info.dir();
        if( !dir.exists() )
        {
            if( !dir.mkpath( "." ) )
            {
                warning() << "Could not create directory " << dir;
                source()->transferError(track, i18n( "Could not create directory: %1", dir.path() ) );
                return true; // Attempt to copy/move the next item in m_sources
            }
        }
        if( src == dest) {
        //no changes, so leave the database alone, and don't erase anything
            return true; // Attempt to copy/move the next item in m_sources
        }
        //we should only move it directly if we're moving within the same collection
        else if( isGoingToRemoveSources() && source()->collection() == collection() )
        {
            debug() << "moving!";
            job = KIO::file_move( src, dest, -1, flags );
        }
        else
        {
        //later on in the case that remove is called, the file will be deleted because we didn't apply moveByDestination to the track
            job = KIO::file_copy( src, dest, -1, flags );
        }
        if( job )   //just to be safe
        {
            connect( job, SIGNAL( result( KJob* ) ), SLOT( slotJobFinished( KJob* ) ) );
            connect( job, SIGNAL( result( KJob* ) ), m_transferjob, SLOT( slotJobFinished( KJob* ) ) );
            m_transferjob->addSubjob( job );
            QString name = track->prettyName();
            if( track->artist() )
                name = QString( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

            m_transferjob->emitInfo( i18n( "Transferring: %1", name ) );
            m_jobs.insert( job, track );
            return true;
        }
        debug() << "JOB NULL OMG!11";
    }
    return false;
}

bool SqlCollectionLocation::startNextRemoveJob()
{
    DEBUG_BLOCK
    while ( !m_removetracks.isEmpty() )
    {
        Meta::TrackPtr track = m_removetracks.takeFirst();
        KUrl src = m_originalUrls[track];

        if( src == track->playableUrl() ) // src == dst
            break;

        KIO::DeleteJob *job = 0;

        src.cleanPath();
        debug() << "deleting  " << src;
        KIO::JobFlags flags = KIO::HideProgressInfo;
        job = KIO::del( src, flags );
        if( job )   //just to be safe
        {
            connect( job, SIGNAL( result( KJob* ) ), SLOT( slotRemoveJobFinished( KJob* ) ) );
            QString name = track->prettyName();
            if( track->artist() )
                name = QString( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

            The::statusBar()->newProgressOperation( job, i18n( "Removing: %1", name ) );
            m_removejobs.insert( job, track );
            return true;
        }
        break;
    }
    return false;
}


TransferJob::TransferJob( SqlCollectionLocation * location )
    : KCompositeJob( 0 )
    , m_location( location )
    , m_killed( false )
{
    setCapabilities( KJob::Killable );
}

bool TransferJob::addSubjob(KJob* job)
{
    return KCompositeJob::addSubjob(job);
}

void TransferJob::emitInfo(const QString& message)
{
    emit infoMessage( this, message );
}


void TransferJob::start()
{
    DEBUG_BLOCK
    if( m_location == 0 )
    {
        setError( 1 );
        setErrorText( "Location is null!" );
        emitResult();
        return;
    }
    QTimer::singleShot( 0, this, SLOT( doWork() ) );
}

void TransferJob::doWork()
{
    DEBUG_BLOCK
    setTotalAmount(  KJob::Files, m_location->m_sources.size() );
    setProcessedAmount( KJob::Files, 0 );
    if( !m_location->startNextJob() )
    {
        if( hasSubjobs() )
            emitResult();
    }
}

void TransferJob::slotJobFinished( KJob* job )
{
    Q_UNUSED( job );
    DEBUG_BLOCK
    if( m_killed )
    {
        debug() << "slotJobFinished entered, but it should be killed!";
        return;
    }
    setProcessedAmount( KJob::Files, processedAmount( KJob::Files ) + 1 );
    emitPercent( processedAmount( KJob::Files ), totalAmount( KJob::Files ) );
    debug() << "processed" << processedAmount( KJob::Files ) << " totalAmount" << totalAmount( KJob::Files );
    if( !m_location->startNextJob() )
    {
        // don't quit if there are still subjobs
        if( hasSubjobs() )
            emitResult();
    }
}

bool TransferJob::doKill()
{
    DEBUG_BLOCK
    m_killed = true;
    foreach( KJob* job, subjobs() )
    {
        job->kill();
    }
    clearSubjobs();
    return KJob::doKill();
}

#include "SqlCollectionLocation.moc"
