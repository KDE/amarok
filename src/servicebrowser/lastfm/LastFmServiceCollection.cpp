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
#include "servicemetabase.h"

// libunicorn includes
#include "libUnicorn/WebService/Request.h"
#include "WebService.h"

#include "support/MemoryQueryMaker.h"

#include <KLocale>

using namespace Meta;

LastFmServiceCollection::LastFmServiceCollection( const QString& userName )
    : ServiceDynamicCollection( "last.fm", "last.fm" ) 
{

    m_userName = userName;

    ServiceGenre * userStreams = new ServiceGenre( userName +"'s Streams" );
    GenrePtr userStreamsPtr( userStreams );
    addGenre( userStreamsPtr->name(), userStreamsPtr );

    ServiceGenre * globalTags = new ServiceGenre( "Global Tags" );
    GenrePtr globalTagsPtr( globalTags );
    addGenre( globalTagsPtr->name(), globalTagsPtr );

    m_neighbors = new ServiceGenre( i18n( "Neighbors' Radio" ) );
    GenrePtr neighborsPtr( m_neighbors );
    addGenre( neighborsPtr->name(), neighborsPtr );

    m_friends = new ServiceGenre( i18n( "Friends' Radio" ) );
    GenrePtr friendsPtr( m_friends );
    addGenre( friendsPtr->name(), friendsPtr );

    QStringList lastfmPersonal;
    lastfmPersonal << "personal" << "neighbours" << "loved";

    foreach( QString station, lastfmPersonal ) {
        LastFm::Track * track = new LastFm::Track( "lastfm://user/" + userName + "/" + station );
        TrackPtr trackPtr( track );
        userStreams->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }

    QStringList lastfmGenres;
    lastfmGenres << "Alternative" << "Ambient" << "Chill Out" << "Classical"<< "Dance"
            << "Electronica" << "Favorites" << "Heavy Metal" << "Hip Hop" << "Indie Rock"
            << "Industrial" << "Japanese" << "Pop" << "Psytrance" << "Rap" << "Rock"
            << "Soundtrack" << "Techno" << "Trance";

    
    foreach( QString genre, lastfmGenres ) {
        LastFm::Track * track = new LastFm::Track( "lastfm://globaltags/" + genre );
        TrackPtr trackPtr( track );
        globalTags->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }

    connect( The::webService(), SIGNAL( neighbours( WeightedStringList ) ), SLOT( slotAddNeighbours( WeightedStringList ) ) );
    connect( The::webService(), SIGNAL( friends( QStringList ) ), SLOT( slotAddFriends( QStringList ) ) );

    NeighboursRequest *nr = new NeighboursRequest();
    nr->start();

    FriendsRequest *fr = new FriendsRequest();
    fr->start();

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
        TrackPtr trackPtr( track );
        m_neighbors->addTrack( trackPtr );
        addTrack( trackPtr->name(), trackPtr );
    }
}

void LastFmServiceCollection::slotAddFriends( QStringList list )
{
    foreach( const QString &string, list )
    {
        LastFm::Track *track = new LastFm::Track( "lastfm://user/" + string + "/personal" );
        TrackPtr trackPtr( track );
        m_friends->addTrack( trackPtr );
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

