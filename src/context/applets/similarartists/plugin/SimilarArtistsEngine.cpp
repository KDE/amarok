/***************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
* Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
* Copyright (c) 2024 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

#include "amarokurls/AmarokUrl.h"
#include "EngineController.h"
#include "collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "services/lastfm/LastFmServiceConfig.h"

#include <QUrlQuery>
#include <QXmlStreamReader>


SimilarArtistsEngine::SimilarArtistsEngine( QObject *parent )
    : QObject( parent )
    , m_maxArtists( 5 )
    , m_model( new SimilarArtistModel( this ) )
    , m_artistInfoQueryInProcess( false )
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
        m_model->clearAll();
        return false;
    }
    else   //valid artist
    {
        // wee make a request only if the artist is different
        if( force || ( newArtist != m_model->currentTarget() ) )
        {
            // if the artist has changed
            m_model->setCurrentTarget( newArtist );
            similarArtistsRequest( newArtist );
            Q_EMIT targetChanged();
            return true;
        }
    }
    return false;
}

void
SimilarArtistsEngine::searchLocalCollection( const QString &artistName )
{
    DEBUG_BLOCK

    // -- search the collection for albums with the artist
    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setAutoDelete( true );
    qm->addFilter( Meta::valArtist, artistName, true, true );
    qm->setAlbumQueryMode( Collections::QueryMaker::AllAlbums );
    qm->setQueryType( Collections::QueryMaker::Album );
    qm->limitMaxResultSize( 3 );

    connect( qm, &Collections::QueryMaker::newAlbumsReady,
             this, &SimilarArtistsEngine::resultReady, Qt::QueuedConnection );

    m_lastQueryMaker = qm;
    m_queriedArtist = artistName;
    qm->run();
}

void SimilarArtistsEngine::resultReady( const Meta::AlbumList &albums )
{
    if( sender() != m_lastQueryMaker )
        return;

    if( albums.length() > 0 )
        debug() << "found albums in our own collection";
    for( auto album : albums )
    {
        if( album->name().isEmpty() )
            continue;

        QUrl cover = album->imageLocation( 200 );
        if( cover != QUrl() )
        {
            m_model->setCover( m_queriedArtist, cover );
            return;
        }
    }
}

void
SimilarArtistsEngine::navigateToArtist( const QString &artist )
{
    AmarokUrl url;
    url.setCommand( QStringLiteral("navigate") );
    url.setPath( QStringLiteral("collections") );
    url.setArg( QStringLiteral("filter"), QStringLiteral("artist:\"") + AmarokUrl::escape( artist ) + QLatin1Char('\"') );
    url.run();
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
SimilarArtistsEngine::artistInfoRequest( const QString &artistName )
{
    if(m_artistInfoQueryInProcess)
        return;
    m_artistInfoQueryInProcess = true;
    // we generate the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( QStringLiteral("https") );
    url.setHost( QStringLiteral("ws.audioscrobbler.com") );
    url.setPath( QStringLiteral("/2.0/") );

    QUrlQuery query;
    query.addQueryItem( QStringLiteral("method"), QStringLiteral("artist.getInfo") );
    query.addQueryItem( QStringLiteral("api_key"), QLatin1String(Amarok::lastfmApiKey()) );
    query.addQueryItem( QStringLiteral("artist"), artistName );
    if( !LastFmServiceConfig::instance()->username().isNull() )
    query.addQueryItem( QStringLiteral("username"), LastFmServiceConfig::instance()->username() );
    url.setQuery( query );

    The::networkAccessManager()->getData( url, this, &SimilarArtistsEngine::parseArtistInfo );
}

void
SimilarArtistsEngine::parseSimilarArtists( const QUrl &url, const QByteArray &data,
                                           NetworkAccessManagerProxy::Error e )
{
    if( e.code != QNetworkReply::NoError )
    {
        m_model->clearAll();
        warning() << "Failed to parse similar artists xml:" << url << e.description;
        return;
    }

    if( data.isEmpty() )
        return;

    QXmlStreamReader xml( data );
    m_model->fillFromXml( xml );
}

void
SimilarArtistsEngine::parseArtistInfo( const QUrl &url, const QByteArray &data,
                                       NetworkAccessManagerProxy::Error e )
{
    if( e.code != QNetworkReply::NoError )
    {
        m_model->clearAll();
        warning() << "Failed to parse artist info xml:" << url << e.description;
        m_artistInfoQueryInProcess = false;
        return;
    }

    if( data.isEmpty() )
    {
        m_artistInfoQueryInProcess = false;
        return;
    }

    QXmlStreamReader xml( data );
    m_model->fillArtistInfoFromXml( xml );
    m_artistInfoQueryInProcess = false;
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
    Q_EMIT maxArtistsChanged();
}

QString
SimilarArtistsEngine::currentTarget() const
{
    return m_model->currentTarget();
}
