/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#ifndef AMAROK_PLAYLISTFILELOADERJOB_H
#define AMAROK_PLAYLISTFILELOADERJOB_H

#include "core-impl/playlists/types/file/PlaylistFile.h"

#include <KTemporaryFile>
#include <ThreadWeaver/Job>

class KJob;

namespace Playlists
{
    /**
     * Allows threading during playlist file loading. Auto-deletes when its work is done.
     */
    class PlaylistFileLoaderJob :public QObject,  public ThreadWeaver::Job
    {
        Q_OBJECT

        public:
            PlaylistFileLoaderJob( const PlaylistFilePtr &playlist );

        protected:
            void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread = 0) Q_DECL_OVERRIDE;

        Q_SIGNALS:
            /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
            void done(ThreadWeaver::JobPointer);
            /** This job has failed. */
            void failed(ThreadWeaver::JobPointer);

        private Q_SLOTS:
            void slotDonwloadFinished( KJob *job );

            /**
             * Responsible for notification of finished loading
             */
            void slotDone();

        private:
            PlaylistFilePtr m_playlist;
            KTemporaryFile m_tempFile;
            QString m_actualPlaylistFile; // path to local playlist file to actually load
            QSemaphore m_downloadSemaphore;
    };
}

#endif
