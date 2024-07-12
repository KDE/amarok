/***************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
* Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
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

#define DEBUG_PREFIX "SimilarArtistsEngine"

#include "SimilarArtistsEngine.h"
#include "SimilarArtist.h"

#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QUrlQuery>
#include <QXmlStreamReader>


SimilarArtistsEngine::SimilarArtistsEngine( QObject *parent )
    : QObject( parent )
    , m_maxArtists( 5 )
{
    EngineController *engine = The::engineController();
    connect( engine, &EngineController::trackChanged, this, &SimilarArtistsEngine::update );
    connect( engine, &EngineController::trackMetadataChanged, this, &SimilarArtistsEngine::update );
}

SimilarArtistsEngine::~SimilarArtistsEngine()
{
}

bool
SimilarArtistsEngine::update( bool force )
{
    QString newArtist;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track )
    {
        if( Meta::ArtistPtr artistPtr = track->artist() )
            newArtist = artistPtr->name();
    }

    if( newArtist.isEmpty() )
    {
        m_artist.clear();
        m_similarArtists.clear();
        Q_EMIT similarArtistsChanged();
        return false;
    }
    else   //valid artist
    {
        // wee make a request only if the artist is different
        if( force || (newArtist != m_artist) )
        {
            // if the artist has changed
            m_artist = newArtist;
            similarArtistsRequest( m_artist );
            return true;
        }
    }
    return false;
}

void
SimilarArtistsEngine::similarArtistsRequest( const QString &artistName )
{
    // we generate the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( QStringLiteral("https") );
    url.setHost( QStringLiteral("ws.audioscrobbler.com") );
    url.setPath( QStringLiteral("/2.0/") );

    QUrlQuery query;
    query.addQueryItem( QStringLiteral("method"), QStringLiteral("artist.getSimilar") );
    query.addQueryItem( QStringLiteral("api_key"), QLatin1String(Amarok::lastfmApiKey()) );
    query.addQueryItem( QStringLiteral("artist"), artistName );
    query.addQueryItem( QStringLiteral("limit"),  QString::number( m_maxArtists ) );
    url.setQuery( query );

    The::networkAccessManager()->getData( url, this, &SimilarArtistsEngine::parseSimilarArtists );
}

void
SimilarArtistsEngine::parseSimilarArtists( const QUrl &url, const QByteArray &data,
                                           NetworkAccessManagerProxy::Error e )
{
    if( e.code != QNetworkReply::NoError )
    {
        m_similarArtists.clear();
        warning() << "Failed to parse similar artists xml:" << url << e.description;
        Q_EMIT similarArtistsChanged();
        return;
    }

    if( data.isEmpty() )
        return;

    QXmlStreamReader xml( data );
    SimilarArtist::List saList = SimilarArtist::listFromXml( xml );
    debug() << "Found" << saList.size() << "similar artists to" << m_artist;
    for( const auto &a : saList )
    {
        m_similarArtists << a->name();
    }
    Q_EMIT similarArtistsChanged();
}

int
SimilarArtistsEngine::maximumArtists() const
{
    return m_maxArtists;
}

void
SimilarArtistsEngine::setMaximumArtists( int number )
{
    m_maxArtists = number;
}

QString
SimilarArtistsEngine::artist() const
{
    return m_artist;
}

void
SimilarArtistsEngine::setArtist( const QString &name )
{
    m_artist = name;
    update();
}
