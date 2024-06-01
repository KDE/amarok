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

#include "SynchronizationAdapter.h"

#include "MetaValues.h"
#include "core/support/Debug.h"
#include "core/support/SemaphoreReleaser.h"
#include "services/lastfm/SynchronizationTrack.h"

#include <KLocalizedString>

#include <QNetworkReply>

#include <Library.h>
#include <XmlQuery.h>

const int SynchronizationAdapter::s_entriesPerQuery( 200 );

SynchronizationAdapter::SynchronizationAdapter( const LastFmServiceConfigPtr &config )
    : m_config( config )
{
    connect( this, &SynchronizationAdapter::startArtistSearch, this, &SynchronizationAdapter::slotStartArtistSearch );
    connect( this, &SynchronizationAdapter::startTrackSearch, this, &SynchronizationAdapter::slotStartTrackSearch );
    connect( this, &SynchronizationAdapter::startTagSearch, this, &SynchronizationAdapter::slotStartTagSearch );
}

SynchronizationAdapter::~SynchronizationAdapter()
{
}

QString
SynchronizationAdapter::id() const
{
    return "lastfm";
}

QString
SynchronizationAdapter::prettyName() const
{
    return i18n( "Last.fm" );
}

QString
SynchronizationAdapter::description() const
{
    return i18nc( "description of the Last.fm statistics synchronization provider",
                  "slows down track matching" );
}

QIcon
SynchronizationAdapter::icon() const
{
    return QIcon::fromTheme( "view-services-lastfm-amarok" );
}

qint64
SynchronizationAdapter::reliableTrackMetaData() const
{
    return Meta::valArtist | Meta::valAlbum | Meta::valTitle;
}

qint64
SynchronizationAdapter::writableTrackStatsData() const
{
    bool useRating = m_config->useFancyRatingTags();
    return ( useRating ? Meta::valRating : 0 ) | Meta::valLabel;
}

StatSyncing::Provider::Preference
SynchronizationAdapter::defaultPreference()
{
    return StatSyncing::Provider::Never; // don't overload Last.fm servers
}

QSet<QString>
SynchronizationAdapter::artists()
{
    DEBUG_BLOCK
    Q_ASSERT( m_semaphore.available() == 0 );
    Q_EMIT startArtistSearch( 1 ); // Last.fm indexes from 1

    m_semaphore.acquire();
    QSet<QString> ret = m_artists;
    m_artists.clear(); // save memory
    debug() << __PRETTY_FUNCTION__ << ret.count() << "artists total";
    return ret;
}

StatSyncing::TrackList
SynchronizationAdapter::artistTracks( const QString &artistName )
{
    /* This method should match track artists case-sensitively, but we don't do it.
     * Last.fm webservice returns only the preferred capitalisation in artists(), so no
     * duplicates threat us. */
    Q_ASSERT( m_semaphore.available() == 0 );
    Q_EMIT startTrackSearch( artistName, 1 ); // Last.fm indexes from 1

    m_semaphore.acquire();
    debug() << __PRETTY_FUNCTION__ << m_tracks.count() << "tracks from" << artistName
            << m_tagQueue.count() << "of them have tags";

    // fetch tags
    QMutableListIterator<StatSyncing::TrackPtr> it( m_tagQueue );
    while( it.hasNext() )
    {
        StatSyncing::TrackPtr track = it.next();
        Q_EMIT startTagSearch( track->artist(), track->name() );
        m_semaphore.acquire();
        it.remove();
    }

    StatSyncing::TrackList ret = m_tracks;
    m_tracks.clear(); // save memory
    m_tagQueue.clear(); // paranoia
    return ret;
}

void
SynchronizationAdapter::slotStartArtistSearch( int page )
{
    QString user = m_config->username();
    QNetworkReply *reply = lastfm::Library::getArtists( user, s_entriesPerQuery, page );
    connect( reply, &QNetworkReply::finished, this, &SynchronizationAdapter::slotArtistsReceived );
}

void
SynchronizationAdapter::slotStartTrackSearch( QString artistName, int page )
{
    lastfm::Artist artist( artistName );
    QString user = m_config->username();
    QNetworkReply *reply = lastfm::Library::getTracks( user, artist, s_entriesPerQuery, page );
    connect( reply, &QNetworkReply::finished, this, &SynchronizationAdapter::slotTracksReceived );
}

void
SynchronizationAdapter::slotStartTagSearch( QString artistName, QString trackName )
{
    lastfm::MutableTrack track;
    track.setArtist( artistName );
    track.setTitle( trackName );
    QNetworkReply *reply = track.getTags();
    connect( reply, &QNetworkReply::finished, this, &SynchronizationAdapter::slotTagsReceived );
}

void
SynchronizationAdapter::slotArtistsReceived()
{
    SemaphoreReleaser releaser( &m_semaphore );
    QNetworkReply *reply =  qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot cast sender to QNetworkReply. (?)";
        return;
    }
    reply->deleteLater();

    lastfm::XmlQuery lfm;
    if( !lfm.parse( reply->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "Error parsing Last.fm reply:" << lfm.parseError().message();
        return;
    }
    lastfm::XmlQuery artists = lfm[ "artists" ];
    bool ok = false;
    int page = artists.attribute( "page" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read page number";
        return;
    }
    int totalPages = artists.attribute( "totalPages" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read total number or pages";
        return;
    }
    debug() << __PRETTY_FUNCTION__ << "page" << page << "of" << totalPages;

    // following is based on lastfm::Artist::list():
    foreach( const lastfm::XmlQuery &xq, lfm.children( "artist" ) )
    {
        lastfm::Artist artist( xq );
        m_artists.insert( artist.name() );
    }

    // Last.fm indexes from 1!
    if( page < totalPages )
    {
        releaser.dontRelease(); // don't release the semaphore yet
        Q_EMIT startArtistSearch( page + 1 );
    }
}

void
SynchronizationAdapter::slotTracksReceived()
{
    SemaphoreReleaser releaser( &m_semaphore );
    QNetworkReply *reply =  qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot cast sender to QNetworkReply. (?)";
        return;
    }
    reply->deleteLater();

    lastfm::XmlQuery lfm;
    if( !lfm.parse( reply->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "Error parsing Last.fm reply:" << lfm.parseError().message();
        return;
    }
    lastfm::XmlQuery tracks = lfm[ "tracks" ];
    bool ok = false;
    int page = tracks.attribute( "page" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read page number";
        return;
    }
    int totalPages = tracks.attribute( "totalPages" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read total number or pages";
        return;
    }
    QString searchedArtist = tracks.attribute( "artist" );
    if( searchedArtist.isEmpty() )
    {
        warning() << __PRETTY_FUNCTION__ << "searchedArtist in Last.fm reply is empty";
        return;
    }

    // following is based on lastfm::Track::list():
    foreach( const lastfm::XmlQuery &xq, lfm.children( "track" ) )
    {
        QString name = xq[ "name" ].text();
        int playCount = xq[ "playcount" ].text().toInt();
        int tagCount = xq[ "tagcount" ].text().toInt();
        QString artist = xq[ "artist" ][ "name" ].text();
        QString album = xq[ "album" ][ "name" ].text();

        bool useRatings = m_config->useFancyRatingTags();
        StatSyncing::TrackPtr track( new SynchronizationTrack( artist, album, name,
                                                               playCount, useRatings ) );
        m_tracks.append( track );
        if( tagCount > 0 )
            m_tagQueue.append( track );
    }

    // Last.fm indexes from 1!
    if( page < totalPages )
    {
        releaser.dontRelease(); // don't release the semaphore yet
        Q_EMIT startTrackSearch( searchedArtist, page + 1 );
    }
}

void
SynchronizationAdapter::slotTagsReceived()
{
    SemaphoreReleaser releaser( &m_semaphore );
    QNetworkReply *reply =  qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot cast sender to QNetworkReply. (?)";
        return;
    }
    reply->deleteLater();

    lastfm::XmlQuery lfm;
    if( !lfm.parse( reply->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "Error parsing Last.fm reply:" << lfm.parseError().message();
        return;
    }
    QSet<QString> tags;
    foreach( const lastfm::XmlQuery &xq, lfm.children( "tag" ) )
    {
        tags.insert( xq[ "name" ].text() );
    }
    Q_ASSERT( !m_tagQueue.isEmpty() );
    SynchronizationTrack *track = dynamic_cast<SynchronizationTrack *>( m_tagQueue.first().data() );
    Q_ASSERT( track );
    track->parseAndSaveLastFmTags( tags );
}
