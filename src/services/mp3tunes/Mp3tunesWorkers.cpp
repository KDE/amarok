/****************************************************************************************
 * Copyright (c) 2007,2008 Casey Link <unnamedrambler@gmail.com>                        *
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

#include "Mp3tunesWorkers.h"

#include "Debug.h"
#include "Mp3tunesMeta.h"
#include "statusbar/StatusBar.h"

#include <QStringList>

Mp3tunesLoginWorker::Mp3tunesLoginWorker( Mp3tunesLocker* locker,
                                          const QString & username,
                                          const QString & password )
    : ThreadWeaver::Job()
    , m_locker( locker )
    , m_sessionId()
    , m_username( username )
    , m_password( password )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
}

Mp3tunesLoginWorker::~Mp3tunesLoginWorker()
{
}

void Mp3tunesLoginWorker::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Calling Locker login..";
        m_sessionId = m_locker->login(m_username, m_password);
        debug() << "Login Complete. SessionId = " << m_sessionId;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesLoginWorker::completeJob()
{
    DEBUG_BLOCK
    debug() << "Login Job complete";
    emit( finishedLogin( m_sessionId ) );
    deleteLater();
}
/* ARTIST FETCHER */
Mp3tunesArtistFetcher::Mp3tunesArtistFetcher( Mp3tunesLocker * locker )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
}

Mp3tunesArtistFetcher::~Mp3tunesArtistFetcher()
{
}

void Mp3tunesArtistFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Artist Fetch Start";
        QList<Mp3tunesLockerArtist> list = m_locker->artists();
        debug() << "Artist Fetch End. Total artists: " << list.count();
        m_artists = list;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesArtistFetcher::completeJob()
{
    emit( artistsFetched( m_artists ) );
    deleteLater();
}

/*  ALBUM w/ Artist Id FETCHER */
Mp3tunesAlbumWithArtistIdFetcher::Mp3tunesAlbumWithArtistIdFetcher( Mp3tunesLocker * locker, int artistId )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    m_artistId = artistId;
}

Mp3tunesAlbumWithArtistIdFetcher::~Mp3tunesAlbumWithArtistIdFetcher()
{
}

void Mp3tunesAlbumWithArtistIdFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Album Fetch Start";
        QList<Mp3tunesLockerAlbum> list = m_locker->albumsWithArtistId( m_artistId );
        debug() << "Album Fetch End. Total albums: " << list.count();
        m_albums = list;
    } else {
                debug() << "Locker is NULL";
    }
}

void Mp3tunesAlbumWithArtistIdFetcher::completeJob()
{
    emit( albumsFetched( m_albums ) );
    deleteLater();
}

/*  TRACK w/ albumId FETCHER */
Mp3tunesTrackWithAlbumIdFetcher::Mp3tunesTrackWithAlbumIdFetcher( Mp3tunesLocker * locker, int albumId )
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    debug() << "Constructor albumId: " << albumId;
    m_albumId = albumId;
}

Mp3tunesTrackWithAlbumIdFetcher::~Mp3tunesTrackWithAlbumIdFetcher()
{
}

void Mp3tunesTrackWithAlbumIdFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Track Fetch Start for album " << m_albumId;
        QList<Mp3tunesLockerTrack> list = m_locker->tracksWithAlbumId( m_albumId );
        debug() << "Track Fetch End. Total tracks: " << list.count();
        m_tracks = list;
    } else {
            debug() << "Locker is NULL";
    }
}

void Mp3tunesTrackWithAlbumIdFetcher::completeJob()
{
    DEBUG_BLOCK
    emit( tracksFetched( m_tracks ) );
    deleteLater();
}

/*  TRACK w/ artistId FETCHER */
Mp3tunesTrackWithArtistIdFetcher::Mp3tunesTrackWithArtistIdFetcher( Mp3tunesLocker * locker, int artistId )
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    debug() << "Constructor artistId: " << artistId;
    m_artistId = artistId;
}

Mp3tunesTrackWithArtistIdFetcher::~Mp3tunesTrackWithArtistIdFetcher()
{
}

void Mp3tunesTrackWithArtistIdFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Track Fetch Start for artist " << m_artistId;
        QList<Mp3tunesLockerTrack> list = m_locker->tracksWithArtistId( m_artistId );
        debug() << "Track Fetch End. Total tracks: " << list.count();
        m_tracks = list;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesTrackWithArtistIdFetcher::completeJob()
{
    DEBUG_BLOCK
    emit( tracksFetched( m_tracks ) );
    deleteLater();
}

/*  SEARCH MONKEY */
Mp3tunesSearchMonkey::Mp3tunesSearchMonkey( Mp3tunesLocker * locker, QString query, int searchFor )
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    m_searchFor = searchFor;
    m_query = query;
}

Mp3tunesSearchMonkey::~Mp3tunesSearchMonkey()
{}

void Mp3tunesSearchMonkey::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        Mp3tunesSearchResult container;
        debug() << "Searching query: " << m_query << "    bitmask: " << m_searchFor;
        container.searchFor = (Mp3tunesSearchResult::SearchType) m_searchFor;
        if( !m_locker->search(container, m_query) )
        {
            //TODO proper error handling
            debug() << "!!!Search Failed query: " << m_query << "    bitmask: " << m_searchFor;
        }
        m_result = container;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesSearchMonkey::completeJob()
{
    DEBUG_BLOCK
    emit( searchComplete( m_result.artistList ) );
    emit( searchComplete( m_result.albumList ) );
    emit( searchComplete( m_result.trackList ) );
    deleteLater();
}

/*  SIMPLE UPLOADER */
Mp3tunesSimpleUploader:: Mp3tunesSimpleUploader( Mp3tunesLocker * locker, QStringList tracklist )
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );

    m_locker = locker;
    m_tracklist = tracklist;

    The::statusBar()->newProgressOperation( this, i18n( "Upload to MP3tunes Initiated" ) )->setMaximum( m_tracklist.count() );
    connect( this, SIGNAL( incrementProgress() ), The::statusBar(), SLOT(
incrementProgress() ), Qt::QueuedConnection );
}

Mp3tunesSimpleUploader::~Mp3tunesSimpleUploader()
{
    DEBUG_BLOCK
    The::statusBar()->endProgressOperation( this );
}

void Mp3tunesSimpleUploader::run()
{
    DEBUG_BLOCK
    if(m_locker == 0)
        return;
    if(m_tracklist.count() == 0)
    {
        debug() << "Track list was empty.";
        return;
    }

    debug() << "Starting upload of " << m_tracklist.count() << " tracks.";
    int progress = 1;
    foreach(const QString &track, m_tracklist) {
        QString msg = i18n( "Uploading Track %1/%2", progress, m_tracklist.count() );
        debug() << msg;
        The::statusBar()->setProgressStatus( this, msg );
        emit ( incrementProgress() );
        debug() << "Uploading: " << track;

        bool result = false;
        if( track.startsWith( "http" ) )
        {
            debug() << "Remote file.";
            result = m_locker->lockerLoad( track );
        } else {
            debug() << "Local file.";
            result = m_locker->uploadTrack( track );
        }

        if(result) {
            debug() << "Uploaded Succeeded.";
        } else {
            debug() << "Uploaded Failed.";
            debug() << "Error msg: " << m_locker->errorMessage();
        }
        progress++;
    }
    debug() << "Upload loop complete";
}

void Mp3tunesSimpleUploader::completeJob()
{
    DEBUG_BLOCK
    emit( uploadComplete() );
    deleteLater();
}

/*  TRACK from filekey FETCHER */
Mp3tunesTrackFromFileKeyFetcher::Mp3tunesTrackFromFileKeyFetcher( Mp3tunesLocker * locker, QString filekey )
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    debug() << "Constructor filekey: " << filekey;
    m_filekey = filekey;
}

Mp3tunesTrackFromFileKeyFetcher::~Mp3tunesTrackFromFileKeyFetcher()
{
}

void Mp3tunesTrackFromFileKeyFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Track Fetch from filekey " << m_filekey;
        Mp3tunesLockerTrack track =  m_locker->trackWithFileKey( m_filekey );
        debug() << "Track Fetch from filekey End.";
        m_track = track;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesTrackFromFileKeyFetcher::completeJob()
{
    DEBUG_BLOCK
    emit( trackFetched( m_track ) );
    deleteLater();
}
