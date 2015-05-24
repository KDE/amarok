/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "IpodCopyTracksJob.h"

#include "IpodMeta.h"
#include "core/collections/QueryMaker.h"
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/transcoding/TranscodingController.h"
#include "MetaTagLib.h"
#include "FileType.h"
#include "transcoding/TranscodingJob.h"

#include <KIO/CopyJob>
#include <KIO/Job>
#include <KMessageBox>

#include <QFile>

#include <gpod/itdb.h>
#include <unistd.h>  // fsync()

IpodCopyTracksJob::IpodCopyTracksJob( const QMap<Meta::TrackPtr,QUrl> &sources,
                                      const QWeakPointer<IpodCollection> &collection,
                                      const Transcoding::Configuration &configuration,
                                      bool goingToRemoveSources )
    : Job()
    , m_coll( collection )
    , m_transcodingConfig( configuration )
    , m_sources( sources )
    , m_aborted( false )
    , m_goingToRemoveSources( goingToRemoveSources )
{
    connect( this, SIGNAL(startDuplicateTrackSearch(Meta::TrackPtr)),
                   SLOT(slotStartDuplicateTrackSearch(Meta::TrackPtr)) );
    connect( this, SIGNAL(startCopyOrTranscodeJob(QUrl,QUrl,bool)),
                   SLOT(slotStartCopyOrTranscodeJob(QUrl,QUrl,bool)) );
    connect( this, SIGNAL(displaySorryDialog()), SLOT(slotDisplaySorryDialog()) );
}

void
IpodCopyTracksJob::run()
{
    if( !m_coll )
        return;  // destructed behind our back
    float totalSafeCapacity = m_coll.data()->totalCapacity() - m_coll.data()->capacityMargin();
    QByteArray mountPoint = QFile::encodeName( m_coll.data()->mountPoint() );
    QString collectionPrettyName = m_coll.data()->prettyName();

    itdb_start_sync( m_coll.data()->m_itdb );
    QMapIterator<Meta::TrackPtr, QUrl> it( m_sources );
    while( it.hasNext() )
    {
        if( m_aborted || !m_coll )
            break;

        it.next();
        Meta::TrackPtr track = it.key();
        QUrl sourceUrl = it.value();
        emit startDuplicateTrackSearch( track );

        // wait for searching to finish:
        m_searchingForDuplicates.acquire( 1 );
        if( m_duplicateTrack )
        {
            trackProcessed( Duplicate, track, m_duplicateTrack );
            continue;
        }

        if( !m_coll )
            break;  // destructed behind our back

            bool isJustCopy = m_transcodingConfig.isJustCopy( track, m_coll.data()->supportedFormats() );

        if( isJustCopy  // if not copying, we catch big files later
            && track->filesize() > totalSafeCapacity - m_coll.data()->usedCapacity() )
        {
            // this is a best effort check, we do one definite one after the file is copied
            debug() << "Refusing to copy" << track->prettyUrl() << "to iPod: there are only"
                    << totalSafeCapacity - m_coll.data()->usedCapacity() << "free bytes (not"
                    << "counting a safety margin) on iPod and track has" << track->filesize()
                    << "bytes.";
            trackProcessed( ExceededingSafeCapacity, track );
            continue;
        }
        QString fileExtension;
        if( isJustCopy )
            fileExtension = track->type();
        else
            fileExtension = Amarok::Components::transcodingController()->format(
                            m_transcodingConfig.encoder() )->fileExtension();
        if( !m_coll.data()->supportedFormats().contains( fileExtension ) )
        {
            m_notPlayableFormats.insert( fileExtension );
            trackProcessed( NotPlayable, track );
            continue;
        }
        QByteArray fakeSrcName( "filename." );  // only for file extension
        fakeSrcName.append( QFile::encodeName( fileExtension ) );

        /* determine destination filename; we cannot use ipodTrack because as it has no itdb
         * (and thus mountpoint) set */
        GError *error = 0;
        gchar *destFilename = itdb_cp_get_dest_filename( 0, mountPoint, fakeSrcName, &error );
        if( error )
        {
            warning() << "Cannot construct iPod track filename:" << error->message;
            g_error_free( error );
            error = 0;
        }
        if( !destFilename )
        {
            trackProcessed( InternalError, track );
            continue;
        }

        // start the physical copying
        QUrl destUrl = QUrl( QFile::decodeName( destFilename ) );
        emit startCopyOrTranscodeJob( sourceUrl, destUrl, isJustCopy );

        // wait for copying to finish:
        m_copying.acquire( 1 );
        /* fsync so that progress bar gives correct info and user doesn't remove the iPod
         * prematurely */
        QFile destFile( QFile::decodeName( destFilename ) );
        if( !destFile.exists() )
        {
            debug() << destFile.fileName() << "does not exist even though we thought we copied it to iPod";
            trackProcessed( CopyingFailed, track );
            continue;
        }
        if( !m_coll )
            break;  // destructed behind our back
        if( m_coll.data()->usedCapacity() > totalSafeCapacity )
        {
            debug() << "We exceeded total safe-to-use capacity on iPod (safe-to-use:"
                    << totalSafeCapacity << "B, used:" << m_coll.data()->usedCapacity()
                    << "): removing copied track from iPod";
            destFile.remove();
            trackProcessed( ExceededingSafeCapacity, track );
            continue;
        }
        // fsync needs a file opened for writing, and without Apped it truncates files (?)
        if( !destFile.open( QIODevice::WriteOnly | QIODevice::Append ) )
        {
            warning() << "Cannot open file copied to ipod (for writing):" << destFile.fileName()
                      << ": removing it";
            destFile.remove();
            trackProcessed( InternalError, track );
            continue;
        }
        if( destFile.size() )
        fsync( destFile.handle() ); // should flush all kernel buffers to disk
        destFile.close();

        // create a new track object by copying meta-data from existing one:
        IpodMeta::Track *ipodTrack = new IpodMeta::Track( track );
        // tell the track it has been copied:
        bool accepted = ipodTrack->finalizeCopying( mountPoint, destFilename );
        g_free( destFilename );
        destFilename = 0;
        if( !accepted )
        {
            debug() << "ipodTrack->finalizeCopying( destFilename )  returned false!";
            delete ipodTrack;
            trackProcessed( InternalError, track );
            continue;
        }
        if( !isJustCopy )
        {
            // we need to reread some metadata in case the file was transcoded
            Meta::FieldHash fields = Meta::Tag::readTags( destFile.fileName() );
            ipodTrack->setBitrate( fields.value( Meta::valBitrate, 0 ).toInt() );
            ipodTrack->setLength( fields.value( Meta::valLength, 0 ).toLongLong() );
            ipodTrack->setSampleRate( fields.value( Meta::valSamplerate, 0 ).toInt() );
            Amarok::FileType type = Amarok::FileType( fields.value( Meta::valFormat, 0 ).toInt() );
            ipodTrack->setType( Amarok::FileTypeSupport::toString( type ) );
            // we retain ReplayGain, tags etc - these shouldn't change; size is read
            // in finalizeCopying()
        }

        // add the track to collection
        if( !m_coll )
        {
            delete ipodTrack;
            break;  // we were waiting for copying, m_coll may got destoryed
        }
        Meta::TrackPtr newTrack = m_coll.data()->addTrack( ipodTrack );
        if( !newTrack )
        {
            destFile.remove();
            trackProcessed( InternalError, track );
            continue;
        }
        trackProcessed( Success, track, newTrack );
    }

    if( m_coll )
        itdb_stop_sync( m_coll.data()->m_itdb );
    emit endProgressOperation( this );

    int sourceSize = m_sources.size();
    int successCount = m_sourceTrackStatus.count( Success );
    int duplicateCount = m_sourceTrackStatus.count( Duplicate );
    QString transferredText;
    if ( m_transcodingConfig.isJustCopy() )
        transferredText = i18ncp( "%2 is collection name", "Transferred one track to %2.",
                                  "Transferred %1 tracks to %2.", successCount, collectionPrettyName );
    else
        transferredText = i18ncp( "%2 is collection name", "Transcoded one track to %2.",
                                  "Transcoded %1 tracks to %2.", successCount, collectionPrettyName );

    if( successCount == sourceSize )
    {
        Amarok::Components::logger()->shortMessage( transferredText );
    }
    else if( m_aborted )
    {
        QString text = i18np( "Transfer aborted. Managed to transfer one track.",
                              "Transfer aborted. Managed to transfer %1 tracks.",
                              successCount );
        Amarok::Components::logger()->longMessage( text );
    }
    else if( successCount + duplicateCount == sourceSize )
    {
        QString text = i18ncp( "%2 is the 'Transferred 123 tracks to Some collection.' message",
            "%2 One track was already there.", "%2 %1 tracks were already there.",
            duplicateCount, transferredText );
        Amarok::Components::logger()->longMessage( text );
    }
    else
    {
        // somethig more severe failed, notify user using a dialog
        emit displaySorryDialog();
    }
}

void
IpodCopyTracksJob::abort()
{
    m_aborted = true;
}

void
IpodCopyTracksJob::slotStartDuplicateTrackSearch( const Meta::TrackPtr &track )
{
    Collections::QueryMaker *qm = m_coll.data()->queryMaker();
    qm->setQueryType( Collections::QueryMaker::Track );

    // we cannot qm->addMatch( track ) - it matches by uidUrl()
    qm->addFilter( Meta::valTitle, track->name(), true, true );
    qm->addMatch( track->album() );
    qm->addMatch( track->artist(), Collections::QueryMaker::TrackArtists );
    qm->addMatch( track->composer() );
    qm->addMatch( track->year() );
    qm->addNumberFilter( Meta::valTrackNr, track->trackNumber(), Collections::QueryMaker::Equals );
    qm->addNumberFilter( Meta::valDiscNr, track->discNumber(), Collections::QueryMaker::Equals );
    // we don't want to match by filesize, track length, filetype etc - these change during
    // transcoding. We don't match album artist because handling of it is inconsistent

    connect( qm, SIGNAL(newResultReady(Meta::TrackList)),
             SLOT(slotDuplicateTrackSearchNewResult(Meta::TrackList)) );
    connect( qm, SIGNAL(queryDone()), SLOT(slotDuplicateTrackSearchQueryDone()) );
    qm->setAutoDelete( true );
    m_duplicateTrack = Meta::TrackPtr(); // reset duplicate track from previous query
    qm->run();
}

void
IpodCopyTracksJob::slotDuplicateTrackSearchNewResult( const Meta::TrackList &tracks )
{
    if( !tracks.isEmpty() )
        // we don't really know which one, but be sure to allow multiple results
        m_duplicateTrack = tracks.last();
}

void
IpodCopyTracksJob::slotDuplicateTrackSearchQueryDone()
{
    m_searchingForDuplicates.release( 1 ); // wakeup run()
}

void
IpodCopyTracksJob::slotStartCopyOrTranscodeJob( const QUrl &sourceUrl, const QUrl &destUrl,
                                                bool isJustCopy )
{
    KJob *job = 0;
    if( isJustCopy )
    {
        if( m_goingToRemoveSources && m_coll &&
            sourceUrl.toLocalFile().startsWith( m_coll.data()->mountPoint() ) )
        {
            // special case for "add orphaned tracks" to either save space and significantly
            // speed-up the process:
            debug() << "Moving from" << sourceUrl << "to" << destUrl;
            job = KIO::file_move( sourceUrl, destUrl, -1, KIO::HideProgressInfo | KIO::Overwrite );
        }
        else
        {
            debug() << "Copying from" << sourceUrl << "to" << destUrl;
            job = KIO::file_copy( sourceUrl, destUrl, -1, KIO::HideProgressInfo | KIO::Overwrite );
        }
    }
    else
    {
        debug() << "Transcoding from" << sourceUrl << "to" << destUrl;
        job = new Transcoding::Job( sourceUrl, destUrl, m_transcodingConfig );
    }
    job->setUiDelegate( 0 ); // be non-interactive
    connect( job, SIGNAL(finished(KJob*)), // we must use this instead of result() to prevent deadlock
             SLOT(slotCopyOrTranscodeJobFinished(KJob*)) );
    job->start();  // no-op for KIO job, but matters for transcoding job
}

void
IpodCopyTracksJob::slotCopyOrTranscodeJobFinished( KJob *job )
{
    if( job->error() != 0 && m_copyErrors.count() < 10 )
        m_copyErrors.insert( job->errorString() );
    m_copying.release( 1 ); // wakeup run()
}

void
IpodCopyTracksJob::slotDisplaySorryDialog()
{
    int sourceSize = m_sources.size();
    int successCount = m_sourceTrackStatus.count( Success );

    // match string with IpodCollectionLocation::prettyLocation()
    QString collName = m_coll ? m_coll.data()->prettyName() : i18n( "Disconnected iPod/iPad/iPhone" );
    QString caption = i18nc( "%1 is collection pretty name, e.g. My Little iPod",
                             "Transferred Tracks to %1", collName );
    QString text;
    if( successCount )
        text = i18np( "One track successfully transferred, but transfer of some other tracks failed.",
                      "%1 tracks successfully transferred, but transfer of some other tracks failed.",
                      successCount );
    else
        text = i18n( "Transfer of tracks failed." );
    QString details;
    int exceededingSafeCapacityCount = m_sourceTrackStatus.count( ExceededingSafeCapacity );
    if( exceededingSafeCapacityCount )
    {
        details += i18np( "One track was not transferred because it would exceed iPod capacity.<br>",
                          "%1 tracks were not transferred because it would exceed iPod capacity.<br>",
                          exceededingSafeCapacityCount );
        QString reservedSpace = m_coll ? KGlobal::locale()->formatByteSize(
            m_coll.data()->capacityMargin(), 1 ) : QString( "???" ); // improbable, don't bother translators
        details += i18nc( "Example of %1 would be: 20.0 MiB",
                          "<i>Amarok reserves %1 on iPod for iTunes database writing.</i><br>",
                          reservedSpace );
    }
    int notPlayableCount = m_sourceTrackStatus.count( NotPlayable );
    if( notPlayableCount )
    {
        QString formats = QStringList( m_notPlayableFormats.toList() ).join( ", " );
        details += i18np( "One track was not copied because it wouldn't be playable - its "
                          " %2 format is unsupported.<br>",
                          "%1 tracks were not copied because they wouldn't be playable - "
                          "they are in unsupported formats (%2).<br>",
                          notPlayableCount, formats );
    }
    int copyingFailedCount = m_sourceTrackStatus.count( CopyingFailed );
    if( copyingFailedCount )
    {
        details += i18np( "Copy/move/transcode of one file failed.<br>",
                          "Copy/move/transcode of %1 files failed.<br>", copyingFailedCount );
    }
    int internalErrorCount = m_sourceTrackStatus.count( InternalError );
    if( internalErrorCount )
    {
        details += i18np( "One track was not transferred due to an internal Amarok error.<br>",
                          "%1 tracks were not transferred due to an internal Amarok error.<br>",
                          internalErrorCount );
        details += i18n( "<i>You can find details in Amarok debugging output.</i><br>" );
    }
    if( m_sourceTrackStatus.size() != sourceSize )
    {
        // aborted case was already caught in run()
        details += i18n( "The rest was not transferred because iPod collection disappeared.<br>" );
    }
    if( !m_copyErrors.isEmpty() )
    {
        details += i18nc( "%1 is a list of errors that occurred during copying of tracks",
                          "Error causes: %1<br>", QStringList( m_copyErrors.toList() ).join( "<br>" ) );
    }
    KMessageBox::detailedSorry( 0, text, details, caption );
}

void
IpodCopyTracksJob::trackProcessed( CopiedStatus status, Meta::TrackPtr srcTrack, Meta::TrackPtr destTrack )
{
    m_sourceTrackStatus.insert( status, srcTrack );
    emit incrementProgress();
    emit signalTrackProcessed( srcTrack, destTrack, status );
}

