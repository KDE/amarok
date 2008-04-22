/***************************************************************************
 * copyright            : (C) 2008 Shane King <kde@dontletsstart.com>      *
 *            (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "lastfm"

#include "LastFmServiceCollection.h"
#include "LastFmServiceQueryMaker.h"
#include "meta/LastFmMeta.h"
#include "ServiceMetaBase.h"

// libunicorn includes
#include "libUnicorn/WebService/Request.h"
#include "WebService.h"

#include "support/MemoryQueryMaker.h"

#include <KLocale>

LastFmServiceCollection::LastFmServiceCollection( const QString& userName )
    : ServiceDynamicCollection( 0, "last.fm", "last.fm" )
{

    m_userName = userName;

    Meta::ServiceGenre * userStreams = new Meta::ServiceGenre( userName +"'s Streams" );
    Meta::GenrePtr userStreamsPtr( userStreams );
    addGenre( userStreamsPtr->name(), userStreamsPtr );

    Meta::ServiceGenre * globalTags = new Meta::ServiceGenre( "Global Tags" );
    Meta::GenrePtr globalTagsPtr( globalTags );
    addGenre( globalTagsPtr->name(), globalTagsPtr );

    m_neighbors = new Meta::ServiceGenre( i18n( "Neighbors' Radio" ) );
    Meta::GenrePtr neighborsPtr( m_neighbors );
    addGenre( neighborsPtr->name(), neighborsPtr );

    m_friends = new Meta::ServiceGenre( i18n( "Friends' Radio" ) );
    Meta::GenrePtr friendsPtr( m_friends );
    addGenre( friendsPtr->name(), friendsPtr );

    m_recentlyLoved = new Meta::ServiceGenre( i18n( "Recently Loved Tracks" ) );
    Meta::GenrePtr recentlyLovedPtr( m_recentlyLoved );
    addGenre( recentlyLovedPtr->name(), recentlyLovedPtr );

    m_recentlyPlayed = new Meta::ServiceGenre( i18n( "Recently Played Tracks" ) );
    Meta::GenrePtr recentlyPlayedPtr( m_recentlyPlayed );
    addGenre( recentlyPlayedPtr->name(), recentlyPlayedPtr );

    // Only show these if the user is a subscriber.
    // Note: isSubscriber is a method we added locally to libUnicorn, if libUnicorn gets bumped we may need to readd if last.fm doesn't
    if( The::webService()->isSubscriber() )
    {
        QStringList lastfmPersonal;
        lastfmPersonal << "personal" << "loved";

        foreach( const QString &station, lastfmPersonal )
        {
                LastFm::Track * track = new LastFm::Track( "lastfm://user/" + userName + "/" + station );
                Meta::TrackPtr trackPtr( track );
                userStreams->addTrack( trackPtr );
                addTrack( trackPtr->name(), trackPtr );
        }
    }

    // Neighbors isn't reliant on being a subscriber.
    LastFm::Track * track = new LastFm::Track( "lastfm://user/" + userName + "/" + "neighbours" );
    Meta::TrackPtr trackPtr( track );
    userStreams->addTrack( trackPtr );
    addTrack( trackPtr->name(), trackPtr );

    QStringList lastfmGenres;
    lastfmGenres << "Alternative" << "Ambient" << "Chill Out" << "Classical"<< "Dance"
            << "Electronica" << "Favorites" << "Heavy Metal" << "Hip Hop" << "Indie Rock"
            << "Industrial" << "Japanese" << "Pop" << "Psytrance" << "Rap" << "Rock"
            << "Soundtrack" << "Techno" << "Trance";


    foreach( const QString &genre, lastfmGenres ) {
        LastFm::Track * track = new LastFm::Track( "lastfm://globaltags/" + genre );
        Meta::TrackPtr trackPtr( track );
        globalTags->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }

    connect( The::webService(), SIGNAL( neighbours( WeightedStringList ) ), SLOT( slotAddNeighbours( WeightedStringList ) ) );
    connect( The::webService(), SIGNAL( friends( QStringList ) ), SLOT( slotAddFriends( QStringList ) ) );

    NeighboursRequest *nr = new NeighboursRequest();
    nr->start();

    FriendsRequest *fr = new FriendsRequest();
    fr->start();

    RecentlyLovedTracksRequest *rlt = new RecentlyLovedTracksRequest;
    connect( rlt, SIGNAL( result( Request* ) ), SLOT( slotRecentlyLovedTrackResult( Request* ) ) );
    rlt->start();

    RecentTracksRequest *rt = new RecentTracksRequest;
    connect( rt, SIGNAL( result( Request* ) ), SLOT( slotRecentTrackResult( Request* ) ) );
    rt->start();

    //TODO Automatically add simmilar artist streams for the users favorite artists.
}


LastFmServiceCollection::~LastFmServiceCollection()
{
}


bool
LastFmServiceCollection::possiblyContainsTrack( const KUrl &url ) const
{
    return url.protocol() == "lastfm";
}


Meta::TrackPtr
LastFmServiceCollection::trackForUrl( const KUrl &url )
{
    return Meta::TrackPtr( new LastFm::Track( url.url() ) );
}


QString
LastFmServiceCollection::collectionId() const
{
    return "last.fm";
}


QString
LastFmServiceCollection::prettyName() const
{
    return i18n( "last.fm" );
}

void LastFmServiceCollection::slotAddNeighbours( WeightedStringList list )
{
    QStringList realList = list;
    foreach( const QString &string, realList )
    {
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + string + "/personal" );
        Meta::TrackPtr trackPtr( track );
        m_neighbors->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }
}

void LastFmServiceCollection::slotAddFriends( QStringList list )
{
    foreach( const QString &string, list )
    {
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + string + "/personal" );
        Meta::TrackPtr trackPtr( track );
        m_friends->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }
}

void LastFmServiceCollection::slotRecentlyLovedTrackResult( Request* _r )
{
    RecentlyLovedTracksRequest *r = static_cast<RecentlyLovedTracksRequest*>( _r );

    QList<Track> tracks = r->tracks();
    foreach( Track track, tracks )
    {
        LastFm::Track *metaTrack = new LastFm::Track( track );
        Meta::TrackPtr trackPtr( metaTrack );
        m_recentlyLoved->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }
}

void LastFmServiceCollection::slotRecentTrackResult( Request* _r )
{
    RecentTracksRequest *r = static_cast<RecentTracksRequest*>(_r);

    QList<Track> tracks = r->tracks();
    foreach( Track track, tracks )
    {
        LastFm::Track *metaTrack = new LastFm::Track( track );
        Meta::TrackPtr trackPtr( metaTrack );
        m_recentlyPlayed->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }
}
QueryMaker*
LastFmServiceCollection::queryMaker()
{
    // TODO
    //return new LastFmServiceQueryMaker( this );
    return new MemoryQueryMaker( this, collectionId() );
}

#include "LastFmServiceCollection.moc"

