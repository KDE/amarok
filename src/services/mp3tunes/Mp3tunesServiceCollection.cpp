/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "Mp3tunesServiceCollection.h"

#include "Mp3tunesLockerMeta.h"
#include "Mp3tunesMeta.h"
#include "Mp3tunesServiceCollectionLocation.h"
#include "Mp3tunesServiceQueryMaker.h"
#include "Mp3tunesWorkers.h"
#include "core/support/Debug.h"

#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Job>

#include <QRegularExpression>

using namespace Collections;

Mp3tunesServiceCollection::Mp3tunesServiceCollection( ServiceBase * service, const QString
&sessionId, Mp3tunesLocker * locker )
 : ServiceCollection( service, "Mp3tunesCollection", "Mp3tunesCollection" )
 , m_sessionId( sessionId )
 , m_locker( locker )
{
}


Mp3tunesServiceCollection::~Mp3tunesServiceCollection()
{
}

QueryMaker * Mp3tunesServiceCollection::queryMaker()
{
    return new Mp3tunesServiceQueryMaker( m_locker,  m_sessionId, this );
}

QString Mp3tunesServiceCollection::collectionId() const
{
    return QLatin1String( "MP3tunesLocker" );
}

QString Mp3tunesServiceCollection::prettyName() const
{
    return i18n( "MP3tunes Locker" );
}

bool
Mp3tunesServiceCollection::possiblyContainsTrack(const QUrl &url) const
{
    QRegularExpression rx( "http://content.mp3tunes.com/storage/locker(?:get|play)/(.*)\\?(?:sid|partner_token)=.*" ) ;
    int matches = url.url().indexOf( rx );
    if( matches == -1 ) {
        return false; // not a mp3tunes url
    }
    return true; // for now: if it's a mp3tunes url.. it's likely the track is in the locker
}

Meta::TrackPtr
Mp3tunesServiceCollection::trackForUrl( const QUrl &url )
{
    DEBUG_BLOCK
    if( !m_locker->authenticated() )
        m_locker->login();
    QRegularExpression rx( "http://content.mp3tunes.com/storage/locker(?:get|play)/(.*)\\?(?:sid|partner_token)=.*" ) ;
    QRegularExpressionMatch rmatch = rx.match( url.url() );
    QStringList list = rmatch.capturedTexts();
    QString filekey = list[1]; // Because list[0] is the url itself.
    if ( filekey.isEmpty() ) {
        debug() << "not a track";
        return Meta::TrackPtr(); // It's not an mp3tunes track
    }
    debug() << "filekey: " << filekey;

    Meta::Mp3TunesTrack * serviceTrack = new Meta::Mp3TunesTrack( QString() );
    serviceTrack->setUidUrl( url.url() );

    Mp3tunesTrackFromFileKeyFetcher* trackFetcher = new Mp3tunesTrackFromFileKeyFetcher( m_locker, filekey );
    m_tracksFetching[filekey] = serviceTrack;
    connect( trackFetcher, &Mp3tunesTrackFromFileKeyFetcher::trackFetched, this, &Mp3tunesServiceCollection::trackForUrlComplete );
    //debug() << "Connection complete. Enqueueing..";
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(trackFetcher) );
    //debug() << "m_trackFetcher queue";

    return Meta::TrackPtr( serviceTrack );
}

void Mp3tunesServiceCollection::trackForUrlComplete( Mp3tunesLockerTrack &track )
{
    DEBUG_BLOCK
    //Lets get this thing
    debug() << "got track: " << track.trackTitle();
    QString filekey = track.trackFileKey();
    if( !m_tracksFetching.contains( filekey ) ) {
        debug() << "track not found in QMap";
        return;
    }
    Meta::Mp3TunesTrack * serviceTrack = m_tracksFetching.take( filekey );

    //Building a Meta::Track
    QString title = track.trackTitle().isEmpty() ? "Unknown" :  track.trackTitle();
    serviceTrack->setTitle( title );
    serviceTrack->setId( track.trackId() );
    serviceTrack->setUidUrl( track.playUrl() ); //was: setUrl
    serviceTrack->setDownloadableUrl( track.downloadUrl() );
    serviceTrack->setLength( track.trackLength() );
    serviceTrack->setTrackNumber( track.trackNumber() );
    serviceTrack->setYear( track.albumYear() );

    //Building a Meta::Album
    title = track.albumTitle().isEmpty() ? "Unknown" :  track.albumTitle();
    Meta::Mp3TunesAlbum * serviceAlbum = new Meta::Mp3TunesAlbum( title );
    QString albumIdStr = QString::number( track.albumId() );
    serviceAlbum->setId( track.albumId() );

    QString coverUrl = "http://content.mp3tunes.com/storage/albumartget/<ALBUM_ID>?alternative=1&partner_token=<PARTNER_TOKEN>&sid=<SESSION_ID>";

    coverUrl.replace( "<SESSION_ID>", m_locker->sessionId() );
    coverUrl.replace( "<PARTNER_TOKEN>", m_locker->partnerToken() );
    coverUrl.replace( "<ALBUM_ID>", albumIdStr );

    serviceAlbum->setCoverUrl(coverUrl);
    Meta::AlbumPtr albumPtr( serviceAlbum );
    serviceTrack->setAlbumPtr( albumPtr );

    // Building a Meta::Artist
    QString name = track.artistName().isEmpty() ? "Unknown" :  track.artistName();
    Meta::ServiceArtist * serviceArtist = new Meta::ServiceArtist( name );
    serviceArtist->setId( track.artistId() );
    Meta::ArtistPtr artistPtr( serviceArtist );

    serviceTrack->setArtist( artistPtr );
    serviceAlbum->setArtistName( name );
    serviceAlbum->setAlbumArtist( artistPtr );
//    serviceTrack->update( Meta::TrackPtr( serviceTrack ) );
}

Mp3tunesLocker* Mp3tunesServiceCollection::locker() const
{
    return m_locker;
}

CollectionLocation*
Mp3tunesServiceCollection::location()
{
    return new Mp3tunesServiceCollectionLocation( this );
}



