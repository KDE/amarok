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


QueryMaker*
LastFmServiceCollection::queryMaker()
{
    // TODO
    //return new LastFmServiceQueryMaker( this );
    return new MemoryQueryMaker( this, collectionId() );
}
