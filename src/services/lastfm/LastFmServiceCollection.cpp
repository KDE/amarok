/****************************************************************************************
 * Copyright (c) 2008 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include "meta/LastFmMeta.h"

#include <XmlQuery.h>

using namespace Collections;

LastFmServiceCollection::LastFmServiceCollection( const QString &userName )
    : ServiceCollection( nullptr, QStringLiteral("last.fm"), QStringLiteral("last.fm") )
{
    DEBUG_BLOCK
    Meta::ServiceGenre * userStreams = new Meta::ServiceGenre( i18n( "%1's Streams", userName ) );
    Meta::GenrePtr userStreamsPtr( userStreams );
    addGenre( userStreamsPtr );

    Meta::ServiceGenre * globalTags = new Meta::ServiceGenre( i18n( "Global Tags" ) );
    Meta::GenrePtr globalTagsPtr( globalTags );
    addGenre( globalTagsPtr );

    m_friendsLoved = new Meta::ServiceGenre( i18n( "Friends' Loved Radio" ) );
    Meta::GenrePtr friendsLovedPtr( m_friendsLoved );
    addGenre( friendsLovedPtr );

    m_friendsPersonal = new Meta::ServiceGenre( i18n( "Friends' Personal Radio" ) );
    Meta::GenrePtr friendsPersonalPtr( m_friendsPersonal );
    addGenre( friendsPersonalPtr );

    // Only show these if the user is a subscriber.
    QStringList lastfmPersonal;
    lastfmPersonal << QStringLiteral("personal") << QStringLiteral("recommended") << QStringLiteral("loved");

    for( const QString &station : lastfmPersonal )
    {
        LastFm::Track * track = new LastFm::Track( QStringLiteral("lastfm://user/") + userName + QStringLiteral("/") + station );
        Meta::TrackPtr trackPtr( track );
        userStreams->addTrack( trackPtr );
        addTrack( trackPtr );
    }

    QStringList lastfmGenres;
    lastfmGenres << QStringLiteral("Alternative") << QStringLiteral("Ambient") << QStringLiteral("Chill Out") << QStringLiteral("Classical") << QStringLiteral("Dance")
            << QStringLiteral("Electronica") << QStringLiteral("Favorites") << QStringLiteral("Gospel") << QStringLiteral("Heavy Metal") << QStringLiteral("Hip Hop")
            << QStringLiteral("Indie Rock") << QStringLiteral("Industrial") << QStringLiteral("Japanese") << QStringLiteral("Pop") << QStringLiteral("Psytrance")
            << QStringLiteral("Rap") << QStringLiteral("Rock") << QStringLiteral("Soundtrack") << QStringLiteral("Techno") << QStringLiteral("Trance");


    for( const QString &genre : lastfmGenres )
    {
        LastFm::Track * track = new LastFm::Track( QStringLiteral("lastfm://globaltags/") + genre );
        Meta::TrackPtr trackPtr( track );
        globalTags->addTrack( trackPtr );
        addTrack( trackPtr );
    }

    QMap< QString, QString > params;
    params[ QStringLiteral("method") ] = QStringLiteral("user.getFriends");
    params[ QStringLiteral("user") ] = userName;
    m_jobs[ QStringLiteral("user.getFriends") ] = lastfm::ws::post( params );

    connect( m_jobs[ QStringLiteral("user.getFriends") ], &QNetworkReply::finished, this, &LastFmServiceCollection::slotAddFriendsLoved );
    //connect( m_jobs[ "user.getFriends" ], &QNetworkReply::finished, this, &LastFmServiceCollection::slotAddFriendsPersonal );

    //TODO Automatically add similar artist streams for the users favorite artists.
}

LastFmServiceCollection::~LastFmServiceCollection()
{
    DEBUG_BLOCK
}

bool
LastFmServiceCollection::possiblyContainsTrack( const QUrl &url ) const
{
    return url.scheme() == QStringLiteral("lastfm");
}


Meta::TrackPtr
LastFmServiceCollection::trackForUrl( const QUrl &url )
{
    return Meta::TrackPtr( new LastFm::Track( url.url() ) );
}


QString
LastFmServiceCollection::collectionId() const
{
    return QLatin1String( "last.fm" );
}


QString
LastFmServiceCollection::prettyName() const
{
    return i18n( "Last.fm" );
}

void LastFmServiceCollection::slotAddFriendsLoved()
{
    DEBUG_BLOCK
    if( !m_jobs[ QStringLiteral("user.getFriends") ] )
    {
        debug() << "BAD! got no result object";
        return;
    }
    switch (m_jobs[ QStringLiteral("user.getFriends") ]->error())
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            if( lfm.parse( m_jobs[ QStringLiteral("user.getFriends") ]->readAll() ) )
            {
                for( const lastfm::XmlQuery &e : lfm[ QStringLiteral("friends") ].children( QStringLiteral( "user" ) ) )
                {
                    const QString name = e[ QStringLiteral("name") ].text();
                    LastFm::Track *track = new LastFm::Track( QStringLiteral("lastfm://user/") + name + QStringLiteral("/loved") );
                    Meta::TrackPtr trackPtr( track );
                    m_friendsLoved->addTrack( trackPtr );
                    addTrack( trackPtr );
                }

            }
            else
            {
                debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
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

    m_jobs[ QStringLiteral("user.getFriends") ]->deleteLater();
}

void LastFmServiceCollection::slotAddFriendsPersonal()
{
    DEBUG_BLOCK
    if( !m_jobs[ QStringLiteral("user.getFriends") ] )
    {
        debug() << "BAD! got no result object";
        return;
    }

    switch (m_jobs[ QStringLiteral("user.getFriends") ]->error())
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            if( lfm.parse( m_jobs[ QStringLiteral("user.getFriends") ]->readAll() ) )
            {
                for( const lastfm::XmlQuery &e : lfm[ QStringLiteral("friends") ].children( QStringLiteral("user") ) )
                {
                    const QString name = e[ QStringLiteral("name") ].text();
                    LastFm::Track *track = new LastFm::Track( QStringLiteral("lastfm://user/") + name + QStringLiteral("/personal") );
                    Meta::TrackPtr trackPtr( track );
                    m_friendsPersonal->addTrack( trackPtr );
                    addTrack( trackPtr );
                }

            }
            else
            {
                debug() << "Got exception in parsing from last.fm:" << lfm.parseError().message();
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

    m_jobs[ QStringLiteral("user.getFriends") ]->deleteLater();
}

QueryMaker*
LastFmServiceCollection::queryMaker()
{
    // TODO
    //return new LastFmServiceQueryMaker( this );
    return ServiceCollection::queryMaker();
}


