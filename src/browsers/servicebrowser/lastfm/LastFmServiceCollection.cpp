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
    : ServiceCollection( 0, "last.fm", "last.fm" )
{

    m_userName = userName;

    Meta::ServiceGenre * userStreams = new Meta::ServiceGenre( i18n( "%1's Streams", userName ) );
    Meta::GenrePtr userStreamsPtr( userStreams );
    addGenre( userStreamsPtr );

    Meta::ServiceGenre * globalTags = new Meta::ServiceGenre( i18n( "Global Tags" ) );
    Meta::GenrePtr globalTagsPtr( globalTags );
    addGenre( globalTagsPtr );

    m_neighborsLoved = new Meta::ServiceGenre( i18n( "Neighbors' Loved Radio" ) );
    Meta::GenrePtr neighborsLovedPtr( m_neighborsLoved );
    addGenre( neighborsLovedPtr );

    m_neighborsPersonal = new Meta::ServiceGenre( i18n( "Neighbors' Personal Radio" ) );
    Meta::GenrePtr neighborsPersonalPtr( m_neighborsPersonal );
    addGenre( neighborsPersonalPtr );

    m_friendsLoved = new Meta::ServiceGenre( i18n( "Friends' Loved Radio" ) );
    Meta::GenrePtr friendsLovedPtr( m_friendsLoved );
    addGenre( friendsLovedPtr );

    m_friendsPersonal = new Meta::ServiceGenre( i18n( "Friends' Personal Radio" ) );
    Meta::GenrePtr friendsPersonalPtr( m_friendsPersonal );
    addGenre( friendsPersonalPtr );

    m_recentlyLoved = new Meta::ServiceGenre( i18n( "Recently Loved Tracks" ) );
    Meta::GenrePtr recentlyLovedPtr( m_recentlyLoved );
    addGenre( recentlyLovedPtr );

    m_recentlyPlayed = new Meta::ServiceGenre( i18n( "Recently Played Tracks" ) );
    Meta::GenrePtr recentlyPlayedPtr( m_recentlyPlayed );
    addGenre( recentlyPlayedPtr );

    // Only show these if the user is a subscriber.
    // Note: isSubscriber is a method we added locally to libUnicorn, if libUnicorn gets bumped we may need to readd if last.fm doesn't
    QStringList lastfmPersonal;
    lastfmPersonal << "personal" << "loved" << "neighbours";

    foreach( const QString &station, lastfmPersonal )
    {
            LastFm::Track * track = new LastFm::Track( "lastfm://user/" + userName + "/" + station );
            Meta::TrackPtr trackPtr( track );
            userStreams->addTrack( trackPtr );
            addTrack( trackPtr );
    }

    QStringList lastfmGenres;
    lastfmGenres << "Alternative" << "Ambient" << "Chill Out" << "Classical"<< "Dance"
            << "Electronica" << "Favorites" << "Heavy Metal" << "Hip Hop" << "Indie Rock"
            << "Industrial" << "Japanese" << "Pop" << "Psytrance" << "Rap" << "Rock"
            << "Soundtrack" << "Techno" << "Trance";


    foreach( const QString &genre, lastfmGenres ) {
        LastFm::Track * track = new LastFm::Track( "lastfm://globaltags/" + genre );
        Meta::TrackPtr trackPtr( track );
        globalTags->addTrack( trackPtr );
        addTrack( trackPtr );
    }

    connect( The::webService(), SIGNAL( neighbours( WeightedStringList ) ), SLOT( slotAddNeighboursLoved( WeightedStringList ) ) );
    connect( The::webService(), SIGNAL( neighbours( WeightedStringList ) ), SLOT( slotAddNeighboursPersonal( WeightedStringList ) ) );
    connect( The::webService(), SIGNAL( friends( QStringList ) ), SLOT( slotAddFriendsLoved( QStringList ) ) );
    connect( The::webService(), SIGNAL( friends( QStringList ) ), SLOT( slotAddFriendsPersonal( QStringList ) ) );

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

void LastFmServiceCollection::slotAddNeighboursLoved( WeightedStringList list )
{
    QStringList realList = list;
    foreach( const QString &string, realList )
    {
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + string + "/loved" );
        Meta::TrackPtr trackPtr( track );
        m_neighborsLoved->addTrack( trackPtr );
        addTrack( trackPtr );
    }
}

void LastFmServiceCollection::slotAddNeighboursPersonal( WeightedStringList list )
{
    QStringList realList = list;
    foreach( const QString &string, realList )
    {
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + string + "/personal" );
        Meta::TrackPtr trackPtr( track );
        m_neighborsPersonal->addTrack( trackPtr );
        addTrack( trackPtr );
    }
}

void LastFmServiceCollection::slotAddFriendsLoved( QStringList list )
{
    foreach( const QString &string, list )
    {
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + string + "/loved" );
        Meta::TrackPtr trackPtr( track );
        m_friendsLoved->addTrack( trackPtr );
        addTrack( trackPtr );
    }
}

void LastFmServiceCollection::slotAddFriendsPersonal( QStringList list )
{
    foreach( const QString &string, list )
    {
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + string + "/personal" );
        Meta::TrackPtr trackPtr( track );
        m_friendsPersonal->addTrack( trackPtr );
        addTrack( trackPtr );
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
        addTrack( trackPtr );
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
        addTrack( trackPtr );
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

