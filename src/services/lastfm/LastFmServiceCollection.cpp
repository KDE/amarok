/***************************************************************************
 * copyright            : (C) 2008 Shane King <kde@dontletsstart.com>      *
 *            (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>      *
 *            (C) 2008 Leo Franchi <lfranchi@gmail.com>                    *
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

#include "collection/support/MemoryQueryMaker.h"

#include <lastfm/ws/WsRequestBuilder.h>
#include <lastfm/ws/WsReply.h>
#include <lastfm/ws/WsKeys.h>

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
            << "Electronica" << "Favorites" << "Gospel" << "Heavy Metal" << "Hip Hop"
            << "Indie Rock" << "Industrial" << "Japanese" << "Pop" << "Psytrance"
            << "Rap" << "Rock" << "Soundtrack" << "Techno" << "Trance";


    foreach( const QString &genre, lastfmGenres ) {
        LastFm::Track * track = new LastFm::Track( "lastfm://globaltags/" + genre );
        Meta::TrackPtr trackPtr( track );
        globalTags->addTrack( trackPtr );
        addTrack( trackPtr );
    }

    WsReply* reply = WsRequestBuilder( "user.getNeighbours" )
                        .add( "user", userName )
                        .add( "api_key", QString( Ws::ApiKey ) )
                        .get();

    connect( reply, SIGNAL( finished( WsReply* ) ), this, SLOT( slotAddNeighboursLoved( WsReply* ) ) );
    connect( reply, SIGNAL( finished( WsReply* ) ), this, SLOT( slotAddNeighboursPersonal( WsReply* ) ) );
    
    reply = WsRequestBuilder( "user.getFriends" )
    .add( "user", userName )
    .add( "api_key", QString( Ws::ApiKey ) )
    .get();
    
    connect( reply, SIGNAL( finished( WsReply* ) ), this, SLOT( slotAddFriendsLoved( WsReply* ) ) );
    connect( reply, SIGNAL( finished( WsReply* ) ), this, SLOT( slotAddFriendsPersonal( WsReply* ) ) );
    
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

void LastFmServiceCollection::slotAddNeighboursLoved( WsReply* reply )
{
    DEBUG_BLOCK
    // iterate through each neighbour
    foreach( CoreDomElement e, reply->lfm()[ "neighbours" ].children( "user" ) )
    {
        QString name = e[ "name" ].text();
        debug() << "got neighbour!!! - " << name;
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/loved" );
        Meta::TrackPtr trackPtr( track );
        m_neighborsLoved->addTrack( trackPtr );
        addTrack( trackPtr );
    }
}

void LastFmServiceCollection::slotAddNeighboursPersonal( WsReply* reply )
{
    DEBUG_BLOCK
    // iterate through each neighbour
    foreach( CoreDomElement e, reply->lfm()[ "neighbours" ].children( "user" ) )
    {
        QString name = e[ "name" ].text();
        debug() << "got neighbour!!! - " << name;
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/personal" );
        Meta::TrackPtr trackPtr( track );
        m_neighborsPersonal->addTrack( trackPtr );
        addTrack( trackPtr );
    }
}

void LastFmServiceCollection::slotAddFriendsLoved( WsReply* reply )
{
    DEBUG_BLOCK
    // iterate through each friend
    foreach( CoreDomElement e, reply->lfm()[ "friends" ].children( "user" ) )
    {
        QString name = e[ "name" ].text();
        debug() << "got friend!!! - " << name;
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/loved" );
        Meta::TrackPtr trackPtr( track );
        m_friendsLoved->addTrack( trackPtr );
        addTrack( trackPtr );
    }
}

void LastFmServiceCollection::slotAddFriendsPersonal( WsReply* reply )
{
    DEBUG_BLOCK
    // iterate through each friend
    foreach( CoreDomElement e, reply->lfm()[ "friends" ].children( "user" ) )
    {
        QString name = e[ "name" ].text();
        debug() << "got neighbour!!! - " << name;
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/personal" );
        Meta::TrackPtr trackPtr( track );
        m_friendsPersonal->addTrack( trackPtr );
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

