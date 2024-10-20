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
#include "core-impl/playlists/types/file/PlaylistFileLoaderJob.h"

#include "core/meta/Meta.h"
#include "core/playlists/PlaylistFormat.h"
#include "core/logger/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/support/SemaphoreReleaser.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KIO/FileCopyJob>

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>
#include <QUrl>

using namespace Playlists;

PlaylistFileLoaderJob::PlaylistFileLoaderJob( const PlaylistFilePtr &playlist )
    : QObject()
    , m_playlist( playlist )
{
    connect( this, &PlaylistFileLoaderJob::done, this, &PlaylistFileLoaderJob::slotDone );

    // we must handle remove downloading here as KIO is coupled with GUI as is not
    // designed to work from another thread
    QUrl url( playlist->uidUrl() );
    // KIO::file_copy in KF5 needs scheme
    if (url.isRelative() && url.host().isEmpty()) {
        url.setScheme(QStringLiteral("file"));
    }
    if( url.isLocalFile() )
    {
        m_actualPlaylistFile = url.toLocalFile();
        m_downloadSemaphore.release(); // pretend file "already downloaded"
    }
    else
    {
//         m_tempFile.setFileTemplate( QDir::tempPath() + "/XXXXXX." + Amarok::extension( url.url() ) );
        if( !m_tempFile.open() )
        {
            Amarok::Logger::longMessage(
                    i18n( "Could not create a temporary file to download playlist." ) );
            m_downloadSemaphore.release(); // prevent deadlock
            return;
        }

        KIO::FileCopyJob *job = KIO::file_copy( url , QUrl::fromLocalFile(m_tempFile.fileName()), 0774,
                                                KIO::Overwrite | KIO::HideProgressInfo );
        Amarok::Logger::newProgressOperation( job,
                i18n("Downloading remote playlist" ) );
        if( playlist->isLoadingAsync() )
            // job is started automatically by KIO
            connect( job, &KIO::FileCopyJob::finished, this, &PlaylistFileLoaderJob::slotDownloadFinished );
        else
        {
            job->exec();
            slotDownloadFinished( job );
        }
    }
}

void
PlaylistFileLoaderJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    SemaphoreReleaser releaser( m_playlist->isLoadingAsync() ? nullptr : &m_playlist->m_loadingDone );
    m_downloadSemaphore.acquire(); // wait for possible download to finish
    if( m_actualPlaylistFile.isEmpty() )
        return; // previous error, already reported

    QFile file( m_actualPlaylistFile );
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        Amarok::Logger::longMessage( i18nc( "%1 is file path",
                "Cannot read playlist from %1", m_actualPlaylistFile ), Amarok::Logger::Error );
        return;
    }

    QByteArray content = file.readAll();
    file.close();

    m_playlist->load( content );
}

void
PlaylistFileLoaderJob::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
PlaylistFileLoaderJob::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
PlaylistFileLoaderJob::slotDownloadFinished( KJob *job )
{
    if( job->error() )
    {
        using namespace Amarok;
        warning() << job->errorString();
    }
    else
        m_actualPlaylistFile = m_tempFile.fileName();
    m_downloadSemaphore.release();
}

void
PlaylistFileLoaderJob::slotDone()
{
    m_playlist->notifyObserversTracksLoaded();
}
