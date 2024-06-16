/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TrackLoader.h"

#include "core/playlists/PlaylistFormat.h"
#include "core/support/Debug.h"
#include "core-impl/meta/file/File.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core-impl/meta/multi/MultiTrack.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"

#include <KIO/Job>
#include <KFileItem>

#include <QFileInfo>
#include <QTimer>

TrackLoader::TrackLoader( Flags flags, int timeout )
    : m_status( LoadingTracks )
    , m_flags( flags )
    , m_timeout( timeout )
{
}

TrackLoader::~TrackLoader()
{
}

void
TrackLoader::init( const QUrl &url )
{
    init( QList<QUrl>() << url );
}

void
TrackLoader::init( const QList<QUrl> &qurls )
{
    m_sourceUrls = qurls;
    QTimer::singleShot( 0, this, &TrackLoader::processNextSourceUrl );
}

void
TrackLoader::init( const Playlists::PlaylistList &playlists )
{
    m_resultPlaylists = playlists;
    // no need to process source urls here, short-cut to result urls (just playlists)
    QTimer::singleShot( 0, this, &TrackLoader::processNextResultUrl );
}

void
TrackLoader::processNextSourceUrl()
{
    if( m_sourceUrls.isEmpty() )
    {
        QTimer::singleShot( 0, this, &TrackLoader::processNextResultUrl );
        return;
    }

    QUrl sourceUrl = m_sourceUrls.takeFirst();
    if( !sourceUrl.isValid() )
    {
        error() << "Url is invalid:" << sourceUrl;
        QTimer::singleShot( 0, this, &TrackLoader::processNextSourceUrl );
        return;
    }
    if( sourceUrl.isLocalFile() && QFileInfo( sourceUrl.toLocalFile() ).isDir() )
    {
        // KJobs delete themselves
        KIO::ListJob *lister = KIO::listRecursive( sourceUrl );
        connect( lister, &KIO::ListJob::result, this, &TrackLoader::processNextSourceUrl );
        connect( lister, &KIO::ListJob::entries, this, &TrackLoader::directoryListResults );
        return;
    }
    else
        m_resultUrls.append( sourceUrl );

    QTimer::singleShot( 0, this, &TrackLoader::processNextSourceUrl );
}

void
TrackLoader::directoryListResults( KIO::Job *job, const KIO::UDSEntryList &list )
{
    //dfaure says that job->redirectionUrl().isValid() ? job->redirectionUrl() : job->url(); might be needed
    //but to wait until an issue is actually found, since it might take more work
    const QUrl dir = static_cast<KIO::SimpleJob *>( job )->url();
    for( const KIO::UDSEntry &entry : list )
    {
        KFileItem item( entry, dir, true, true );
        QUrl url = item.url();
        if( MetaFile::Track::isTrack( url ) )
        {
            auto insertIter = std::upper_bound( m_resultUrls.begin(), m_resultUrls.end(), url, directorySensitiveLessThan );
            m_resultUrls.insert( insertIter, url );
        }
    }
}

void
TrackLoader::processNextResultUrl()
{
    using namespace Playlists;
    if( !m_resultPlaylists.isEmpty() )
    {
        PlaylistPtr playlist = m_resultPlaylists.takeFirst();
        PlaylistObserver::subscribeTo( playlist );
        playlist->triggerTrackLoad(); // playlist track loading is on demand.
        // will trigger tracksLoaded() which in turn calls processNextResultUrl(),
        // therefore we shouldn't call trigger processNextResultUrl() here:
        return;
    }

    if( m_resultUrls.isEmpty() )
    {
        mayFinish();
        return;
    }

    QUrl resultUrl = m_resultUrls.takeFirst();
    if( isPlaylist( resultUrl ) )
    {
        PlaylistFilePtr playlist = loadPlaylistFile( resultUrl );
        if( playlist )
        {
            PlaylistObserver::subscribeTo( PlaylistPtr::staticCast( playlist ) );
            playlist->triggerTrackLoad(); // playlist track loading is on demand.
            // will trigger tracksLoaded() which in turn calls processNextResultUrl(),
            // therefore we shouldn't call trigger processNextResultUrl() here:
            return;
        }
        else
            warning() << __PRETTY_FUNCTION__ << "cannot load playlist" << resultUrl;
    }
    else if( MetaFile::Track::isTrack( resultUrl ) )
    {
        MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( resultUrl ) );
        proxyTrack->setTitle( resultUrl.fileName() ); // set temporary name
        Meta::TrackPtr track( proxyTrack.data() );
        m_tracks << Meta::TrackPtr( track );

        if( m_flags.testFlag( FullMetadataRequired ) && !proxyTrack->isResolved() )
        {
            m_unresolvedTracks.insert( track );
            Observer::subscribeTo( track );
        }
    }
    else
        warning() << __PRETTY_FUNCTION__ << resultUrl
                  << "is neither a playlist or a track, skipping";

                  QTimer::singleShot( 0, this, &TrackLoader::processNextResultUrl );
}

void
TrackLoader::tracksLoaded( Playlists::PlaylistPtr playlist )
{
    // this method needs to be thread-safe!

    // some playlists used to Q_EMIT tracksLoaded() in ->tracks(), prevent infinite
    // recursion by unsubscribing early
    PlaylistObserver::unsubscribeFrom( playlist );

    // accessing m_tracks is thread-safe as nothing else is happening in this class in
    // the main thread while we are waiting for tracksLoaded() to trigger:
    Meta::TrackList tracks = playlist->tracks();
    if( m_flags.testFlag( FullMetadataRequired ) )
    {
        for( const Meta::TrackPtr &track : tracks )
        {
            MetaProxy::TrackPtr proxyTrack = MetaProxy::TrackPtr::dynamicCast( track );
            if( !proxyTrack )
            {
                debug() << __PRETTY_FUNCTION__ << "strange, playlist" << playlist->name()
                        << "doesn't use MetaProxy::Tracks";
                continue;
            }
            if( !proxyTrack->isResolved() )
            {
                m_unresolvedTracks.insert( track );
                Observer::subscribeTo( track );
            }
        }
    }

    static const QSet<QString> remoteProtocols = QSet<QString>()
            << "http" << "https" << "mms" << "smb"; // consider unifying with CollectionManager::trackForUrl()
    if( m_flags.testFlag( RemotePlaylistsAreStreams ) && tracks.count() > 1
        && remoteProtocols.contains( playlist->uidUrl().scheme() ) )
    {
        m_tracks << Meta::TrackPtr( new Meta::MultiTrack( playlist ) );
    }
    else
        m_tracks << tracks;

    // this also ensures that processNextResultUrl() will resume in the main thread
    QTimer::singleShot( 0, this, &TrackLoader::processNextResultUrl );
}

void
TrackLoader::metadataChanged( const Meta::TrackPtr &track )
{
    // first metadataChanged() from a MetaProxy::Track means that it has found the real track
    bool isEmpty;
    {
        QMutexLocker locker( &m_unresolvedTracksMutex );
        m_unresolvedTracks.remove( track );
        isEmpty = m_unresolvedTracks.isEmpty();
    }

    Observer::unsubscribeFrom( track );
    if( m_status == MayFinish && isEmpty )
        QTimer::singleShot( 0, this, &TrackLoader::finish );
}

void
TrackLoader::mayFinish()
{
    m_status = MayFinish;
    bool isEmpty;
    {
        QMutexLocker locker( &m_unresolvedTracksMutex );
        isEmpty = m_unresolvedTracks.isEmpty();
    }
    if( isEmpty )
    {
        finish();
        return;
    }

    // we must wait for tracks to resolve, but with a timeout
    QTimer::singleShot( m_timeout, this, &TrackLoader::finish );
}

void
TrackLoader::finish()
{
    // prevent double Q_EMIT of finished(), race between singleshot QTimers from mayFinish()
    // and metadataChanged()
    if( m_status != MayFinish )
        return;

    m_status = Finished;
    Q_EMIT finished( m_tracks );
    deleteLater();
}

bool
TrackLoader::directorySensitiveLessThan( const QUrl &left, const QUrl &right )
{
    QString leftDir = left.adjusted(QUrl::RemoveFilename).path();
    QString rightDir = right.adjusted(QUrl::RemoveFilename).path();

    // filter out tracks from same directories:
    if( leftDir == rightDir )
        return QString::localeAwareCompare( left.fileName(), right.fileName() ) < 0;

    // left is "/a/b/c/", right is "/a/b/"
    if( leftDir.startsWith( rightDir ) )
        return true; // we sort directories above files
    // left is "/a/b/", right is "/a/b/c/"
    if( rightDir.startsWith( leftDir ) )
        return false;

    return QString::localeAwareCompare( leftDir, rightDir ) < 0;
}
