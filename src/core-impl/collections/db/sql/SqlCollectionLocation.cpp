/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2010 Teo Mrnjavac <teo@kde.org>                                        *
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

#define DEBUG_PREFIX "SqlCollectionLocation"

#include "SqlCollectionLocation.h"

#include "core/collections/CollectionLocationDelegate.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/interfaces/Logger.h"
#include "core/collections/support/SqlStorage.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core-impl/collections/db/ScanManager.h"
#include "MountPointManager.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "transcoding/TranscodingJob.h"
#include "core/transcoding/TranscodingController.h"
#include <shared/MetaTagLib.h> // for getting the uid

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <kdiskfreespaceinfo.h>
#include <kjob.h>
#include <KSharedPtr>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/deletejob.h>


using namespace Collections;

SqlCollectionLocation::SqlCollectionLocation( SqlCollection *collection )
    : CollectionLocation( collection )
    , m_collection( collection )
    , m_delegateFactory( 0 )
    , m_overwriteFiles( false )
    , m_transferjob( 0 )
{
    //nothing to do
}

SqlCollectionLocation::~SqlCollectionLocation()
{
    //nothing to do
    delete m_delegateFactory;
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
    // TODO: This function is also called when removing files to check
    //  if the tracks can be removed. In such a case we should not check the space

    // The collection is writable if there exists a path that has more than
    // 500 MB free space.
    bool path_exists_with_space = false;
    bool path_exists_writable = false;
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
            path_exists_writable = true;
	debug() << "\tpath_exists_writable" << path_exists_writable;
	debug() << "\tpath_exists_with_space" << path_exists_with_space;
    }
    return path_exists_with_space && path_exists_writable;
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
    Q_ASSERT( track );

    if( track->inCollection() &&
        track->collection()->collectionId() == m_collection->collectionId() )
    {
        bool removed;
        KUrl src = track->playableUrl();
        if( isGoingToRemoveSources() ) // is organize operation?
        {
            SqlCollectionLocation* destinationloc = qobject_cast<SqlCollectionLocation*>( destination() );
            if( destinationloc )
            {
                src = destinationloc->m_originalUrls[track];
                if( src == track->playableUrl() )
                    return false;
            }
        }
        // we are going to delete it from the database only if is no longer on disk
        removed = !QFile::exists( src.path() );
        if( removed )
            static_cast<Meta::SqlTrack*>(const_cast<Meta::Track*>(track.data()))->remove();

        return removed;
    }
    else
    {
        debug() << "Remove Failed";
        return false;
    }
}

bool
SqlCollectionLocation::insert( const Meta::TrackPtr &track, const QString &url )
{
    debug() << "SqlCollectionLocation::insert from:"<<track->playableUrl()<<"to"<<url;

    if( !QFile::exists( url ) )
        return false; // won't insert bogus file information

    debug() << "File on disk, insert to dbase"<<track->name()<<"uid:"<<track->uidUrl();

    // -- the target url
    SqlRegistry *registry = m_collection->registry();
    int deviceId = m_collection->mountPointManager()->getIdForUrl( url );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, url );
    int directoryId = registry->getDirectory( QFileInfo(url).path() );

    // -- the track uid (we can't use the original one from the old collection)
    Meta::FieldHash fileTags = Meta::Tag::readTags( url );
    QString uid = fileTags.value( Meta::valUniqueId ).toString();

    // -- the the track from the registry
    KSharedPtr<Meta::SqlTrack> metaTrack;
    metaTrack = KSharedPtr<Meta::SqlTrack>::staticCast( registry->getTrackFromUid( uid ) );

    if( metaTrack ) {
        warning() << "Location is inserting a file with the same uid as an already existing one.";
        // TODO: in addition another file with the same url could already exist.
        metaTrack->setUrl( deviceId, rpath, directoryId );

    } else {
        metaTrack = KSharedPtr<Meta::SqlTrack>::staticCast( registry->getTrack( deviceId, rpath, directoryId, uid ) );
    }

    // -- set the values
    metaTrack->setWriteFile( false ); // no need to write the tags back
    metaTrack->beginMetaDataUpdate();

    if( !track->name().isEmpty() )
        metaTrack->setTitle( track->name() );

    if( track->album() )
        metaTrack->setAlbum( track->album()->name() );

    if( track->artist() )
        metaTrack->setArtist( track->artist()->name() );

    if( track->composer() )
        metaTrack->setComposer( track->composer()->name() );

    if( track->year() && track->year()->year() > 0 )
        metaTrack->setYear( track->year()->year() );

    if( track->genre() )
        metaTrack->setGenre( track->genre()->name() );

    // the filetype is not set or in the database.
    // Meta::SqlTrack uses the file extension.

    /* we've already done this
    if( !track->path().isEmpty() )
        metaTrack->setUrl( track->path() );

     and that too
    if( !track->uidUrl().isEmpty() )
        metaTrack->setUidUrl( uid );
    */

    if( track->bpm() > 0 )
        metaTrack->setBpm( track->bpm() );

    if( !track->comment().isEmpty() )
        metaTrack->setComment( track->comment() );

    if( track->length() > 0 )
        metaTrack->setLength( track->length() );

    // the filesize is updated every time after the
    // file is changed. Doesn't make sense to set it.

    if( track->sampleRate() > 0 )
        metaTrack->setSampleRate( track->sampleRate() );

    if( track->bitrate() > 0 )
        metaTrack->setBitrate( track->bitrate() );

    if( track->trackNumber() > 0 )
        metaTrack->setTrackNumber( track->trackNumber() );

    if( track->discNumber() > 0 )
        metaTrack->setDiscNumber( track->discNumber() );

    Meta::ReplayGainTag modes[] = { Meta::ReplayGain_Track_Gain,
        Meta::ReplayGain_Track_Peak,
        Meta::ReplayGain_Album_Gain,
        Meta::ReplayGain_Album_Peak };

    for( int i=0; i<4; i++ )
        if( track->replayGain( modes[i] ) != 0 )
            metaTrack->setReplayGain( modes[i], track->replayGain( modes[i] ) );

    Meta::LabelList labels = track->labels();
    foreach( Meta::LabelPtr label, labels )
        metaTrack->addLabel( label );
    
    Amarok::FileType fileType = Amarok::FileTypeSupport::fileType( track->type() );
    if( fileType != Amarok::Unknown )
        metaTrack->setType( fileType );

    metaTrack->endMetaDataUpdate();

    // Used to be updated after changes commit to prevent crash on NULL pointer access
    // if metaTrack had no album.
    if( track->album() && metaTrack->album() )
    {
        metaTrack->beginMetaDataUpdate();
        
        if( track->album()->hasAlbumArtist() && !metaTrack->album()->hasAlbumArtist() )
            metaTrack->setAlbumArtist( track->album()->albumArtist()->name() );
        
        if( track->album()->hasImage() && !metaTrack->album()->hasImage() )
            metaTrack->album()->setImage( track->album()->image() );
        
        metaTrack->endMetaDataUpdate();
    }

    metaTrack->setWriteFile( true );

    // we have a first shot at the meta data (expecially ratings and playcounts from media
    // collections) but we still need to trigger the collection scanner
    // to get the album and other meta data correct.
    m_collection->scanManager()->delayedIncrementalScan( QFileInfo(url).path() );

    return true;
}

void
SqlCollectionLocation::showDestinationDialog( const Meta::TrackList &tracks,
                                              bool removeSources,
                                              const Transcoding::Configuration &configuration )
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
        // finally, ensure the path is writable
        debug() << ( freeSpace - transferSize );
        if( ( freeSpace - transferSize ) > 1024*1024*500 && info.isWritable() )
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

    OrganizeCollectionDelegate *delegate = m_delegateFactory->createDelegate();
    delegate->setTracks( tracks );
    delegate->setFolders( available_folders );
    delegate->setIsOrganizing( ( collection() == source()->collection() ) );
    delegate->setTranscodingConfiguration( configuration );

    connect( delegate, SIGNAL( accepted() ), SLOT( slotDialogAccepted() ) );
    connect( delegate, SIGNAL( rejected() ), SLOT( slotDialogRejected() ) );
    delegate->show();
}

void
SqlCollectionLocation::slotDialogAccepted()
{
    DEBUG_BLOCK
    sender()->deleteLater();
    OrganizeCollectionDelegate *ocDelegate = qobject_cast<OrganizeCollectionDelegate*>( sender() );
    m_destinations = ocDelegate->destinations();
    m_overwriteFiles = ocDelegate->overwriteDestinations();
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

    Meta::TrackPtr track = m_jobs.value( job );
    if( job->error() )
    {
        //TODO: proper error handling
        warning() << "An error occurred when copying a file: " << job->errorString();
        source()->transferError( track, KIO::buildErrorString( job->error(), job->errorString() ) );
        m_destinations.remove( track );
    }
    else
        source()->transferSuccessful( track );

    m_jobs.remove( job );
    job->deleteLater();

}

void
SqlCollectionLocation::slotRemoveJobFinished( KJob *job )
{
    DEBUG_BLOCK
    Meta::TrackPtr track = m_removejobs.value( job );
    if( job->error() )
    {
        //TODO: proper error handling
        warning() << "An error occurred when removing a file: " << job->errorString();
    }

    // -- remove the track from the database if it's gone
    if( !QFile(track->playableUrl().path()).exists() )
    {
        // Remove the track from the database
        remove( track );

        //we  assume that KIO works correctly...
        transferSuccessful( track );
    }
    else
    {
        transferError( track, KIO::buildErrorString( job->error(), job->errorString() ) );
    }

    m_removejobs.remove( job );
    job->deleteLater();

    if( !startNextRemoveJob() )
    {
        m_collection->scanManager()->unblockScan();
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
        if( QFile::exists( m_destinations[ track ] ) )
            insert( track, m_destinations[ track ] );
        m_originalUrls[track] = track->playableUrl();
    }
    debug () << "m_originalUrls" << m_originalUrls;
    m_collection->scanManager()->unblockScan();
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
        if( QFile::exists( m_destinations[ track ] ) )
            insert( track, m_destinations[ track ] ); // was already copied, so have to insert it in the db
        m_originalUrls[track] = track->playableUrl();
    }
    m_collection->scanManager()->unblockScan();
    abort();
}


void
SqlCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources,
                                             const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    m_collection->scanManager()->blockScan();  //make sure the collection scanner does not run while we are coyping stuff

    m_sources = sources;

    QString statusBarTxt;

    if( destination() == source() )
        statusBarTxt = i18n( "Organizing tracks" );
    else if ( isGoingToRemoveSources() )
        statusBarTxt = i18n( "Moving tracks" );
    else
    {
        if( configuration.encoder() == Transcoding::NULL_CODEC )
            statusBarTxt = i18n( "Copying tracks" );
        else
            statusBarTxt = i18n( "Transcoding tracks" );
    }

    m_transferjob = new TransferJob( this, configuration );
    Amarok::Components::logger()->newProgressOperation( m_transferjob, statusBarTxt, this,
                                                        SLOT(slotTransferJobAborted()) );
    connect( m_transferjob, SIGNAL(result( KJob * )), SLOT(slotTransferJobFinished( KJob * )) );
    m_transferjob->start();
}

void
SqlCollectionLocation::removeUrlsFromCollection(  const Meta::TrackList &sources )
{
    DEBUG_BLOCK

    m_collection->scanManager()->blockScan();  //make sure the collection scanner does not run while we are deleting stuff

    m_removetracks = sources;

    if( !startNextRemoveJob() ) //this signal needs to be called no matter what, even if there are no job finishes to call it
    {
        m_collection->scanManager()->unblockScan();
        slotRemoveOperationFinished();
    }
}

void
SqlCollectionLocation::setOrganizeCollectionDelegateFactory( OrganizeCollectionDelegateFactory *fac )
{
    m_delegateFactory = fac;
}

bool SqlCollectionLocation::startNextJob( const Transcoding::Configuration configuration )
{
    DEBUG_BLOCK
    if( !m_sources.isEmpty() )
    {
        Meta::TrackPtr track = m_sources.keys().first();
        KUrl src = m_sources.take( track );

        KUrl dest = m_destinations[ track ];
        dest.cleanPath();
        src.cleanPath();

        bool hasMoodFile = QFile::exists( moodFile( src ).toLocalFile() );

        if( configuration.encoder() == Transcoding::NULL_CODEC )
            debug() << "copying from " << src << " to " << dest;
        else
            debug() << "transcoding from " << src << " to " << dest;

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

        KIO::JobFlags flags;
        if( configuration.encoder() == Transcoding::NULL_CODEC )
        {
            flags = KIO::HideProgressInfo;
            if( m_overwriteFiles )
            {
                flags |= KIO::Overwrite;
            }
        }

        KJob *job = 0;
        KJob *moodJob = 0;

        if( src.equals( dest ) )
        {
            warning() << "move to itself found: " << info.absoluteFilePath();
            m_transferjob->slotJobFinished( 0 );
            if( m_sources.isEmpty() )
                return false;
            return true;
        }
        else if( isGoingToRemoveSources() && source()->collection() == collection() )
        {
            debug() << "moving!";
            job = KIO::file_move( src, dest, -1, flags );
            if( hasMoodFile )
            {
                KUrl moodSrc = moodFile( src );
                KUrl moodDest = moodFile( dest );
                moodJob = KIO::file_move( moodSrc, moodDest, -1, flags );
            }
        }
        else
        {
            //later on in the case that remove is called, the file will be deleted because we didn't apply moveByDestination to the track
            if( configuration.encoder() == Transcoding::NULL_CODEC )
                job = KIO::file_copy( src, dest, -1, flags );
            else
            {
                QString destPath = dest.path();
                destPath.truncate( dest.path().lastIndexOf( '.' ) + 1 );
                destPath.append( Amarok::Components::transcodingController()->
                                 format( configuration.encoder() )->fileExtension() );
                dest.setPath( destPath );
                job = new Transcoding::Job( src, dest, configuration, this );
                job->start();
            }

            if( hasMoodFile )
            {
                KUrl moodSrc = moodFile( src );
                KUrl moodDest = moodFile( dest );
                moodJob = KIO::file_copy( moodSrc, moodDest, -1, flags );
            }
        }
        if( job )   //just to be safe
        {
            connect( job, SIGNAL( result( KJob* ) ), SLOT( slotJobFinished( KJob* ) ) );
            connect( job, SIGNAL( result( KJob* ) ), m_transferjob, SLOT( slotJobFinished( KJob* ) ) );
            m_transferjob->addSubjob( job );

            if( moodJob )
            {
                connect( moodJob, SIGNAL( result( KJob* ) ), m_transferjob, SLOT( slotJobFinished( KJob* ) ) );
                m_transferjob->addSubjob( moodJob );
            }

            QString name = track->prettyName();
            if( track->artist() )
                name = QString( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

            if( configuration.encoder() == Transcoding::NULL_CODEC )
                m_transferjob->emitInfo( i18n( "Transferring: %1", name ) );
            else
                m_transferjob->emitInfo( i18n( "Transcoding: %1", name ) );
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
        // KUrl src = track->playableUrl();
        KUrl src = track->playableUrl();
        KUrl srcMoodFile = moodFile( src );

        debug() << "isGoingToRemoveSources() " << isGoingToRemoveSources();
        if( isGoingToRemoveSources() && destination() ) // is organize operation?
        {
            SqlCollectionLocation* destinationloc = dynamic_cast<SqlCollectionLocation*>( destination() );

            // src = destinationloc->m_originalUrls[track];
            if( destinationloc && src == destinationloc->m_destinations[track] ) {
                debug() << "src == dst ("<<src<<")";
                continue;
            }
        }

        src.cleanPath();
        debug() << "deleting  " << src;
        KIO::DeleteJob *job = KIO::del( src, KIO::HideProgressInfo );
        if( job )   //just to be safe
        {
            if( QFile::exists( srcMoodFile.toLocalFile() ) )
                KIO::del( srcMoodFile, KIO::HideProgressInfo );
           
            connect( job, SIGNAL( result( KJob* ) ), SLOT( slotRemoveJobFinished( KJob* ) ) );
            QString name = track->prettyName();
            if( track->artist() )
                name = QString( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

            Amarok::Components::logger()->newProgressOperation( job, i18n( "Removing: %1", name ) );
            m_removejobs.insert( job, track );
            return true;
        }
        break;
    }
    return false;
}

KUrl 
SqlCollectionLocation::moodFile( const KUrl &track ) const
{
    KUrl moodPath = track;
    moodPath.setFileName( "." + moodPath.fileName().replace( QRegExp( "(\\.\\w{2,5})$" ), ".mood" ) );
    return moodPath;
}

TransferJob::TransferJob( SqlCollectionLocation * location, const Transcoding::Configuration & configuration )
    : KCompositeJob( 0 )
    , m_location( location )
    , m_killed( false )
    , m_transcodeFormat( configuration )
{
    setCapabilities( KJob::Killable );
    debug() << "TransferJob::TransferJob";
}

bool TransferJob::addSubjob( KJob* job )
{
    connect( job, SIGNAL( processedAmount( KJob *, KJob::Unit, qulonglong ) ),
             this, SLOT( propagateProcessedAmount( KJob *, KJob::Unit, qulonglong ) ) );
    //KCompositeJob::addSubjob doesn't handle progress reporting.
    return KCompositeJob::addSubjob( job );
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
    setTotalAmount( KJob::Files, m_location->m_sources.size() );
    setTotalAmount( KJob::Bytes, m_location->m_sources.size() * 1000 );
    setProcessedAmount( KJob::Files, 0 );
    if( !m_location->startNextJob( m_transcodeFormat ) )
    {
        if( !hasSubjobs() )
            emitResult();
    }
}

void TransferJob::slotJobFinished( KJob* job )
{
    DEBUG_BLOCK
    if( job )
        removeSubjob( job );
    if( m_killed )
    {
        debug() << "slotJobFinished entered, but it should be killed!";
        return;
    }
    setProcessedAmount( KJob::Files, processedAmount( KJob::Files ) + 1 );
    emitPercent( processedAmount( KJob::Files ) * 1000, totalAmount( KJob::Bytes ) );
    debug() << "processed" << processedAmount( KJob::Files ) << " totalAmount" << totalAmount( KJob::Files );
    if( !m_location->startNextJob( m_transcodeFormat ) )
    {
        debug() << "sources empty";
        // don't quit if there are still subjobs
        if( !hasSubjobs() )
            emitResult();
        else
            debug() << "have subjobs";
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

void TransferJob::propagateProcessedAmount( KJob *job, KJob::Unit unit, qulonglong amount ) //SLOT
{
    if( unit == KJob::Bytes )
    {
        qulonglong currentJobAmount = ( static_cast< qreal >( amount ) / job->totalAmount( KJob::Bytes ) ) * 1000;

        setProcessedAmount( KJob::Bytes, processedAmount( KJob::Files ) * 1000 + currentJobAmount );
        emitPercent( processedAmount( KJob::Bytes ), totalAmount( KJob::Bytes ) );
    }
}

#include "SqlCollectionLocation.moc"
