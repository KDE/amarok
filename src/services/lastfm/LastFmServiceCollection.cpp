/****************************************************************************************
 * Copyright (c) 2008 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Leo Franchi <lfranchi@gmail.com>                                  *
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

#define DEBUG_PREFIX "lastfm"

#include "LastFmServiceCollection.h"
#include "LastFmServiceQueryMaker.h"
#include "meta/LastFmMeta.h"
#include "ServiceMetaBase.h"

#include "collection/support/MemoryQueryMaker.h"

#include <lastfm/ws.h>
#include <lastfm/XmlQuery>

#include <QNetworkReply>

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
    QStringList lastfmPersonal;
    lastfmPersonal << "personal" << "recommended" <<  "loved" << "neighbours";

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


    foreach( const QString &genre, lastfmGenres )
    {
        LastFm::Track * track = new LastFm::Track( "lastfm://globaltags/" + genre );
        Meta::TrackPtr trackPtr( track );
        globalTags->addTrack( trackPtr );
        addTrack( trackPtr );
    }

    QMap< QString, QString > params;
    params[ "method" ] = "user.getNeighbours";
    params[ "user" ] = userName;
    m_jobs[ "user.getNeighbours" ] = lastfm::ws::post( params );

    connect( m_jobs[ "user.getNeighbours" ], SIGNAL( finished() ), this, SLOT( slotAddNeighboursLoved() ) );
    //connect( m_jobs[ "user.getNeighbours" ], SIGNAL( finished() ), this, SLOT( slotAddNeighboursPersonal() ) );
    // TODO TMP HACK why do i get exceptions there...!?
    
    params[ "method" ] = "user.getFriends";
    m_jobs[ "user.getFriends" ] = lastfm::ws::post( params );

    connect( m_jobs[ "user.getFriends" ], SIGNAL( finished() ), this, SLOT( slotAddFriendsLoved() ) );
    //connect( m_jobs[ "user.getFriends" ], SIGNAL( finished() ), this, SLOT( slotAddFriendsPersonal() ) );
    
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

void LastFmServiceCollection::slotAddNeighboursLoved()
{
    DEBUG_BLOCK
    if( !m_jobs[ "user.getNeighbours" ] )
    {
        debug() << "BAD! got no result object";
        return;
    }
    switch (m_jobs[ "user.getNeighbours" ]->error())
    {
        case QNetworkReply::NoError:
        {
            // iterate through each neighbour
            try
            {
                lastfm::XmlQuery lfm( m_jobs[ "user.getNeighbours" ]->readAll() );

                foreach( lastfm::XmlQuery e, lfm[ "neighbours" ].children( "user" ) )
                {
                    QString name = e[ "name" ].text();
                    //debug() << "got neighbour!!! - " << name;
                    LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/loved" );
                    Meta::TrackPtr trackPtr( track );
                    m_neighborsLoved->addTrack( trackPtr );
                    addTrack( trackPtr );
                }

            } catch( lastfm::ws::ParseError& e )
            {
                debug() << "Got exception in parsing from last.fm:" << e.what();
            }
            break;
        }

        case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or you typed the password incorrectly.";
            break;

        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }

    m_jobs[ "user.getNeighbours" ]->deleteLater();
}

void LastFmServiceCollection::slotAddNeighboursPersonal()
{
    DEBUG_BLOCK
    switch (m_jobs[ "user.getNeighbours" ]->error())
    {
        case QNetworkReply::NoError:
        {
            // iterate through each neighbour
            try
            {
                if( !m_jobs[ "user.getNeighbours" ] )
                {
                    debug() << "BAD! got no result object";
                    return;
                }
                lastfm::XmlQuery lfm( m_jobs[ "user.getNeighbours" ]->readAll() );

                // iterate through each neighbour
                foreach( lastfm::XmlQuery e, lfm[ "neighbours" ].children( "user" ) )
                {
                    QString name = e[ "name" ].text();
                    debug() << "got neighbour!!! - " << name;
                    LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/personal" );
                    Meta::TrackPtr trackPtr( track );
                    m_neighborsPersonal->addTrack( trackPtr );
                    addTrack( trackPtr );
                }


                // should be safe, as both slots SHOULD get called before we return to the event loop...
                m_jobs[ "user.getNeighbours" ]->deleteLater();
            } catch( lastfm::ws::ParseError& e )
            {
                debug() << "Got exception in parsing from last.fm:" << e.what();
            }
            break;
        }

        case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or you typed the password incorrectly.";
            break;

        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }

}

void LastFmServiceCollection::slotAddFriendsLoved()
{
    DEBUG_BLOCK
    if( !m_jobs[ "user.getFriends" ] )
    {
        debug() << "BAD! got no result object";
        return;
    }
    switch (m_jobs[ "user.getFriends" ]->error())
    {
        case QNetworkReply::NoError:
        {
            try
            {
                lastfm::XmlQuery lfm( m_jobs[ "user.getFriends" ]->readAll() );

                foreach( lastfm::XmlQuery e, lfm[ "friends" ].children( "user" ) )
                {
                    QString name = e[ "name" ].text();
                    LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/loved" );
                    Meta::TrackPtr trackPtr( track );
                    m_friendsLoved->addTrack( trackPtr );
                    addTrack( trackPtr );
                }

            } catch( lastfm::ws::ParseError& e )
            {
                debug() << "Got exception in parsing from last.fm:" << e.what();
            }
            break;
        }

        case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or you typed the password incorrectly.";
            break;

        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }

    m_jobs[ "user.getFriends" ]->deleteLater();
}

void LastFmServiceCollection::slotAddFriendsPersonal()
{
    DEBUG_BLOCK
    if( !m_jobs[ "user.getFriends" ] )
    {
        debug() << "BAD! got no result object";
        return;
    }

    switch (m_jobs[ "user.getFriends" ]->error())
    {
        case QNetworkReply::NoError:
        {
            try
            {
                lastfm::XmlQuery lfm( m_jobs[ "user.getFriends" ]->readAll() );

                foreach( lastfm::XmlQuery e, lfm[ "friends" ].children( "user" ) )
                {
                    QString name = e[ "name" ].text();
                    LastFm::Track *track = new LastFm::Track( "lastfm://user/" + name + "/personal" );
                    Meta::TrackPtr trackPtr( track );
                    m_friendsPersonal->addTrack( trackPtr );
                    addTrack( trackPtr );
                }

            } catch( lastfm::ws::ParseError& e )
            {
                debug() << "Got exception in parsing from last.fm:" << e.what();
            }
            break;
        }

        case QNetworkReply::AuthenticationRequiredError:
            debug() << "Last.fm: errorMessage: Sorry, we don't recognise that username, or you typed the password incorrectly.";
            break;

        default:
            debug() << "Last.fm: errorMessage: There was a problem communicating with the Last.fm services. Please try again later.";
            break;
    }

    m_jobs[ "user.getFriends" ]->deleteLater();
}

QueryMaker*
LastFmServiceCollection::queryMaker()
{
    // TODO
    //return new LastFmServiceQueryMaker( this );
    return new MemoryQueryMaker( this, collectionId() );
}

#include "LastFmServiceCollection.moc"

