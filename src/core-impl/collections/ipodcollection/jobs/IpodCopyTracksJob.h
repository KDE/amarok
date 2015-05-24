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

#ifndef COPYTRACKSJOB_H
#define COPYTRACKSJOB_H

#include "IpodCollection.h"
#include "core/meta/forward_declarations.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <ThreadWeaver/Job>

#include <QMap>
#include <QSemaphore>

class KJob;

class IpodCopyTracksJob : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        enum CopiedStatus {
            Duplicate, /// a track with same meta-data is already in iPod collection
            ExceededingSafeCapacity, /// would exceed "safe" capacity
            NotPlayable, /// track format would not be playable on connected iPod
            CopyingFailed, /// KIO failed to copy the file
            InternalError, /// all other reasons that have no nice user-tellable reason
            Success /// copied successfully
        };

        /**
         * @param goingToRemoveSources whether this is in fact a move operation
         */
        IpodCopyTracksJob( const QMap<Meta::TrackPtr,QUrl> &sources,
                           const QWeakPointer<IpodCollection> &collection,
                           const Transcoding::Configuration &configuration,
                           bool goingToRemoveSources );
        virtual void run();

    public slots:
        void abort();

    signals:
        // a hack to create QueryMaken in a thread with event loop:
        void startDuplicateTrackSearch( const Meta::TrackPtr &track );

        // a hack to create copyjob in a thread with event loop:
        void startCopyOrTranscodeJob( const QUrl &src, const QUrl &dest, bool isJustCopy );

        // a hack to display KMessageBox in a gui thread:
        void displaySorryDialog();

        // signals for progress operation:
        void incrementProgress();
        void endProgressOperation( QObject *obj );
        void totalSteps( int steps ); // not used, defined to keep QObject::conect warning quiet

        /**
         * Signal various track copy statuses back to IpodCollectionLocation
         * @param srcTrack source track, always non-nul
         * @param destTrack destination track on iPod, copied one or existing if
         *                  status == Duplicate; may be null
         * @param status copying status
         */
        void signalTrackProcessed( Meta::TrackPtr srcTrack, Meta::TrackPtr destTrack, IpodCopyTracksJob::CopiedStatus status );

    private slots:
        /// @see startDuplicateTrackSearch()
        void slotStartDuplicateTrackSearch( const Meta::TrackPtr &track );
        void slotDuplicateTrackSearchNewResult( const Meta::TrackList &tracks );
        void slotDuplicateTrackSearchQueryDone();

        /// @see startCopyJob()
        void slotStartCopyOrTranscodeJob( const QUrl &sourceUrl, const QUrl &destUrl,
                                          bool isJustCopy );
        void slotCopyOrTranscodeJobFinished( KJob *job );

        /// @see displaySorryDialog()
        void slotDisplaySorryDialog();

    private:
        void trackProcessed( CopiedStatus status, Meta::TrackPtr srcTrack, Meta::TrackPtr destTrack = Meta::TrackPtr() );

        QWeakPointer<IpodCollection> m_coll;
        Transcoding::Configuration m_transcodingConfig;
        QMap<Meta::TrackPtr,QUrl> m_sources;
        QMultiHash<CopiedStatus, Meta::TrackPtr> m_sourceTrackStatus;
        QSemaphore m_copying;
        QSemaphore m_searchingForDuplicates;
        Meta::TrackPtr m_duplicateTrack;
        bool m_aborted;
        bool m_goingToRemoveSources;
        QSet<QString> m_notPlayableFormats;
        QSet<QString> m_copyErrors;
};

#endif // COPYTRACKSJOB_H
