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

#include "MetaTagLib.h" // for getting the uid
#include "core/collections/CollectionLocationDelegate.h"
#include <core/storage/SqlStorage.h>
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/transcoding/TranscodingController.h"
#include "core-impl/collections/db/MountPointManager.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "core-impl/collections/db/sql/SqlMeta.h"
#include "transcoding/TranscodingJob.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStorageInfo>

#include <KFileItem>
#include <KJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KConfigGroup>
#include <KLocalizedString>

using namespace Collections;

SqlCollectionLocation::SqlCollectionLocation( SqlCollection *collection )
    : CollectionLocation( collection )
    , m_collection( collection )
    , m_delegateFactory( nullptr )
    , m_overwriteFiles( false )
    , m_transferjob( )
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
    // TODO: This function is also called when removing files to check
    //  if the tracks can be removed. In such a case we should not check the space

    // The collection is writable if there exists a path that has more than
    // 500 MB free space.
    bool path_exists_with_space = false;
    bool path_exists_writable = false;
    QStringList folders = actualLocation();
    for( const QString &path : folders )
    {
        float free_space = QStorageInfo( path ).bytesAvailable();
        if( free_space <= 0 )
            continue;
        if( free_space >= 500*1000*1000 ) // ~500 megabytes
            path_exists_with_space = true;

        QFileInfo info( path );
        if( info.isWritable() )
            path_exists_writable = true;
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
        QUrl src = track->playableUrl();
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
SqlCollectionLocation::insert( const Meta::TrackPtr &track, const QString &path )
{
    if( !QFile::exists( path ) )
    {
        warning() << Q_FUNC_INFO << "file" << path << "does not exist, not inserting into db";
        return false;
    }

    // -- the target path
    SqlRegistry *registry = m_collection->registry();
    int deviceId = m_collection->mountPointManager()->getIdForUrl( QUrl::fromLocalFile( path ) );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, path );
    int directoryId = registry->getDirectory( QFileInfo( path ).path() );

    // -- the track uid (we can't use the original one from the old collection)
    Meta::FieldHash fileTags = Meta::Tag::readTags( path );
    QString uid = fileTags.value( Meta::valUniqueId ).toString();
    uid = m_collection->generateUidUrl( uid ); // add the right prefix

    // -- the track from the registry
    Meta::SqlTrackPtr metaTrack;
    metaTrack = Meta::SqlTrackPtr::staticCast( registry->getTrackFromUid( uid ) );

    if( metaTrack )
    {
        warning() << "Location is inserting a file with the same uid as an already existing one.";
        metaTrack->setUrl( deviceId, rpath, directoryId );
    } else
        metaTrack = Meta::SqlTrackPtr::staticCast( registry->getTrack( deviceId, rpath, directoryId, uid ) );

    Meta::ConstStatisticsPtr origStats = track->statistics();

    // -- set the values
    metaTrack->setWriteFile( false ); // no need to write the tags back
    metaTrack->beginUpdate();

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

    if( track->bpm() > 0 )
        metaTrack->setBpm( track->bpm() );
    if( !track->comment().isEmpty() )
        metaTrack->setComment( track->comment() );

    if( origStats->score() > 0 )
        metaTrack->setScore( origStats->score() );
    if( origStats->rating() > 0 )
        metaTrack->setRating( origStats->rating() );

    /* These tags change when transcoding. Prefer to read those from file */
    if( fileTags.value( Meta::valLength, 0 ).toLongLong() > 0 )
        metaTrack->setLength( fileTags.value( Meta::valLength ).value<qint64>() );
    else if( track->length() > 0 )
        metaTrack->setLength( track->length() );
    // the filesize is updated every time after the
    // file is changed. Doesn't make sense to set it.
    if( fileTags.value( Meta::valSamplerate, 0 ).toInt() > 0 )
        metaTrack->setSampleRate( fileTags.value( Meta::valSamplerate ).toInt() );
    else if( track->sampleRate() > 0 )
        metaTrack->setSampleRate( track->sampleRate() );
    if( fileTags.value( Meta::valBitrate, 0 ).toInt() > 0 )
        metaTrack->setBitrate( fileTags.value( Meta::valBitrate ).toInt() );
    else if( track->bitrate() > 0 )
        metaTrack->setBitrate( track->bitrate() );

    // createDate is already set in Track constructor
    if( track->modifyDate().isValid() )
        metaTrack->setModifyDate( track->modifyDate() );

    if( track->trackNumber() > 0 )
        metaTrack->setTrackNumber( track->trackNumber() );
    if( track->discNumber() > 0 )
        metaTrack->setDiscNumber( track->discNumber() );

    if( origStats->lastPlayed().isValid() )
        metaTrack->setLastPlayed( origStats->lastPlayed() );
    if( origStats->firstPlayed().isValid() )
        metaTrack->setFirstPlayed( origStats->firstPlayed() );
    if( origStats->playCount() > 0 )
        metaTrack->setPlayCount( origStats->playCount() );

    Meta::ReplayGainTag modes[] = { Meta::ReplayGain_Track_Gain,
        Meta::ReplayGain_Track_Peak,
        Meta::ReplayGain_Album_Gain,
        Meta::ReplayGain_Album_Peak };
    for( int i=0; i<4; i++ )
        if( track->replayGain( modes[i] ) != 0 )
            metaTrack->setReplayGain( modes[i], track->replayGain( modes[i] ) );

    Meta::LabelList labels = track->labels();
    for( Meta::LabelPtr label : labels )
        metaTrack->addLabel( label );

    if( fileTags.value( Meta::valFormat, int(Amarok::Unknown) ).toInt() != int(Amarok::Unknown) )
        metaTrack->setType( Amarok::FileType( fileTags.value( Meta::valFormat ).toInt() ) );
    else if( Amarok::FileTypeSupport::fileType( track->type() ) != Amarok::Unknown )
        metaTrack->setType( Amarok::FileTypeSupport::fileType( track->type() ) );

    // Used to be updated after changes commit to prevent crash on NULL pointer access
    // if metaTrack had no album.
    if( track->album() && metaTrack->album() )
    {
        if( track->album()->hasAlbumArtist() && !metaTrack->album()->hasAlbumArtist() )
            metaTrack->setAlbumArtist( track->album()->albumArtist()->name() );

        if( track->album()->hasImage() && !metaTrack->album()->hasImage() )
            metaTrack->album()->setImage( track->album()->image() );
    }

    metaTrack->endUpdate();
    metaTrack->setWriteFile( true );

    // we have a first shot at the meta data (especially ratings and playcounts from media
    // collections) but we still need to trigger the collection scanner
    // to get the album and other meta data correct.
    // TODO m_collection->directoryWatcher()->delayedIncrementalScan( QFileInfo(url).path() );

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
    for( Meta::TrackPtr track : tracks )
        transferSize += track->filesize();

    const QStringList actual_folders = actualLocation(); // the folders in the collection
    QStringList available_folders; // the folders which have freespace available
    for(const QString &path : actual_folders)
    {
        if( path.isEmpty() )
            continue;
        debug() << "Path" << path;
        QStorageInfo spaceInfo(path);
        if( !spaceInfo.isValid() )
            continue;

        KIO::filesize_t totalCapacity = spaceInfo.bytesTotal();
        KIO::filesize_t used = totalCapacity - spaceInfo.bytesFree();

        KIO::filesize_t availableSpace = spaceInfo.bytesAvailable();

        debug() << "used:" << used;
        debug() << "total:" << totalCapacity;
        debug() << "Available space" << availableSpace;
        debug() << "transfersize" << transferSize;

        if( totalCapacity <= 0 ) // protect against div by zero
            continue; //How did this happen?

        QFileInfo info( path );

        // since bad things happen when drives become totally full
	// we make sure there is at least ~500MB left
        // finally, ensure the path is writable
        debug() << ( availableSpace - transferSize );
        if( ( availableSpace - transferSize ) > 1024*1024*500 && info.isWritable() )
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
    delegate->setCaption( operationText( configuration ) );

    connect( delegate, &OrganizeCollectionDelegate::accepted, this, &SqlCollectionLocation::slotDialogAccepted );
    connect( delegate, &OrganizeCollectionDelegate::rejected, this, &SqlCollectionLocation::slotDialogRejected );
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
    if( job->error()  && job->error() != KIO::ERR_FILE_ALREADY_EXIST )
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
    for( const Meta::TrackPtr &track : m_destinations.keys() )
    {
        if( QFile::exists( m_destinations[ track ] ) )
            insert( track, m_destinations[ track ] );
        m_originalUrls[track] = track->playableUrl();
    }
    debug () << "m_originalUrls" << m_originalUrls;
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
    for( const Meta::TrackPtr &track : m_destinations.keys() )
    {
        if( QFile::exists( m_destinations[ track ] ) )
            insert( track, m_destinations[ track ] ); // was already copied, so have to insert it in the db
        m_originalUrls[track] = track->playableUrl();
    }
    abort();
}


void
SqlCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                             const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    m_sources = sources;

    QString statusBarTxt = operationInProgressText( configuration, sources.count() );
    m_transferjob = new TransferJob( this, configuration );
    Amarok::Logger::newProgressOperation( m_transferjob, statusBarTxt, this,
                                                        &SqlCollectionLocation::slotTransferJobAborted );
    connect( m_transferjob, &Collections::TransferJob::result,
             this, &SqlCollectionLocation::slotTransferJobFinished );
    m_transferjob->start();
}

void
SqlCollectionLocation::removeUrlsFromCollection(  const Meta::TrackList &sources )
{
    DEBUG_BLOCK

    m_removetracks = sources;

    if( !startNextRemoveJob() ) //this signal needs to be called no matter what, even if there are no job finishes to call it
        slotRemoveOperationFinished();
}

void
SqlCollectionLocation::setOrganizeCollectionDelegateFactory( OrganizeCollectionDelegateFactory *fac )
{
    m_delegateFactory = fac;
}

bool SqlCollectionLocation::startNextJob( const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    if( !m_sources.isEmpty() )
    {
        Meta::TrackPtr track = m_sources.keys().first();
        QUrl src = m_sources.take( track );

        QUrl dest = QUrl::fromLocalFile(m_destinations[ track ]);
        dest.setPath( QDir::cleanPath(dest.path()) );
        src.setPath( QDir::cleanPath(src.path()) );
        // KIO::file_copy in KF5 needs scheme
        if (src.isRelative() && src.host().isEmpty()) {
            src.setScheme("file");
        }

        bool hasMoodFile = QFile::exists( moodFile( src ).toLocalFile() );
        bool isJustCopy = configuration.isJustCopy( track );

        if( isJustCopy )
            debug() << "copying from " << src << " to " << dest;
        else
            debug() << "transcoding from " << src << " to " << dest;

        KFileItem srcInfo( src );
        if( !srcInfo.isFile() )
        {
            warning() << "Source track" << src << "was no file";
            source()->transferError( track, i18n( "Source track does not exist: %1", src.toDisplayString() ) );
            return true; // Attempt to copy/move the next item in m_sources
        }

        QFileInfo destInfo( dest.toLocalFile() );
        QDir dir = destInfo.dir();
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
        if( isJustCopy )
        {
            flags = KIO::HideProgressInfo;
            if( m_overwriteFiles )
            {
                flags |= KIO::Overwrite;
            }
        }

        KJob *job = nullptr;
        KJob *moodJob = nullptr;

        if( src.matches( dest, QUrl::StripTrailingSlash ) )
        {
            warning() << "move to itself found: " << destInfo.absoluteFilePath();
            m_transferjob->slotJobFinished( nullptr );
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
                QUrl moodSrc = moodFile( src );
                QUrl moodDest = moodFile( dest );
                moodJob = KIO::file_move( moodSrc, moodDest, -1, flags );
            }
        }
        else
        {
            //later on in the case that remove is called, the file will be deleted because we didn't apply moveByDestination to the track
            if( isJustCopy )
                job = KIO::file_copy( src, dest, -1, flags );
            else
            {
                QString destPath = dest.path();
                destPath.truncate( dest.path().lastIndexOf( QLatin1Char('.') ) + 1 );
                destPath.append( Amarok::Components::transcodingController()->
                                 format( configuration.encoder() )->fileExtension() );
                dest.setPath( destPath );
                job = new Transcoding::Job( src, dest, configuration, this );
                job->start();
            }

            if( hasMoodFile )
            {
                QUrl moodSrc = moodFile( src );
                QUrl moodDest = moodFile( dest );
                moodJob = KIO::file_copy( moodSrc, moodDest, -1, flags );
            }
        }
        if( job )   //just to be safe
        {
            connect( job, &KJob::result, this, &SqlCollectionLocation::slotJobFinished );
            connect( job, &KJob::result, m_transferjob, &Collections::TransferJob::slotJobFinished );
            m_transferjob->addSubjob( job );

            if( moodJob )
            {
                connect( moodJob, &KJob::result, m_transferjob, &Collections::TransferJob::slotJobFinished );
                m_transferjob->addSubjob( moodJob );
            }

            QString name = track->prettyName();
            if( track->artist() )
                name = QStringLiteral( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

            if( isJustCopy )
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
        // QUrl src = track->playableUrl();
        QUrl src = track->playableUrl();
        QUrl srcMoodFile = moodFile( src );

        debug() << "isGoingToRemoveSources() " << isGoingToRemoveSources();
        if( isGoingToRemoveSources() && destination() ) // is organize operation?
        {
            SqlCollectionLocation* destinationloc = dynamic_cast<SqlCollectionLocation*>( destination() );

            // src = destinationloc->m_originalUrls[track];
            if( destinationloc && src == QUrl::fromUserInput(destinationloc->m_destinations[track]) ) {
                debug() << "src == dst ("<<src<<")";
                continue;
            }
        }

        src.setPath( QDir::cleanPath(src.path()) );
        debug() << "deleting  " << src;
        KIO::DeleteJob *job = KIO::del( src, KIO::HideProgressInfo );
        if( job )   //just to be safe
        {
            if( QFile::exists( srcMoodFile.toLocalFile() ) )
                KIO::del( srcMoodFile, KIO::HideProgressInfo );
           
            connect( job, &KIO::DeleteJob::result, this, &SqlCollectionLocation::slotRemoveJobFinished );
            QString name = track->prettyName();
            if( track->artist() )
                name = QStringLiteral( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

            Amarok::Logger::newProgressOperation( job, i18n( "Removing: %1", name ) );
            m_removejobs.insert( job, track );
            return true;
        }
        break;
    }
    return false;
}

QUrl 
SqlCollectionLocation::moodFile( const QUrl &track ) const
{
    QUrl moodPath = track;
    QString fileName = moodPath.fileName();
    moodPath = moodPath.adjusted(QUrl::RemoveFilename);
    moodPath.setPath(moodPath.path() +  '.' + fileName.replace( QRegularExpression( "(\\.\\w{2,5})$" ), ".mood" ) );
    return moodPath;
}

TransferJob::TransferJob( SqlCollectionLocation * location, const Transcoding::Configuration & configuration )
    : KCompositeJob( nullptr )
    , m_location( location )
    , m_killed( false )
    , m_transcodeFormat( configuration )
{
    setCapabilities( KJob::Killable );
    debug() << "TransferJob::TransferJob";
}

bool TransferJob::addSubjob( KJob* job )
{
    connect( job, SIGNAL(processedAmount(KJob*, KJob::Unit, qulonglong)),
             this, SLOT(propagateProcessedAmount(KJob*, KJob::Unit, qulonglong)) );
    //KCompositeJob::addSubjob doesn't handle progress reporting.
    return KCompositeJob::addSubjob( job );
}

void TransferJob::emitInfo(const QString& message)
{
    Q_EMIT infoMessage( this, message );
}

void TransferJob::slotResult( KJob *job )
{
    // When copying without overwriting some files might already be
    // there and it is not a reason for stopping entire transfer.
    if ( job->error() == KIO::ERR_FILE_ALREADY_EXIST )
        removeSubjob( job );
    else
        KCompositeJob::slotResult( job );
}

void TransferJob::start()
{
    DEBUG_BLOCK
    if( m_location == nullptr )
    {
        setError( 1 );
        setErrorText( "Location is null!" );
        emitResult();
        return;
    }
    QTimer::singleShot( 0, this, &TransferJob::doWork );
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
    for( KJob* job : subjobs() )
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

