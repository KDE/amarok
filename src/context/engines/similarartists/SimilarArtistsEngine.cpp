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

#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QTimer>
#include <QXmlStreamReader>

AMAROK_EXPORT_DATAENGINE( similarArtists, SimilarArtistsEngine )

using namespace Context;

SimilarArtistsEngine::SimilarArtistsEngine( QObject *parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , m_maxArtists( 5 )
{
}

SimilarArtistsEngine::~SimilarArtistsEngine()
{
}

void
SimilarArtistsEngine::init()
{
    EngineController *engine = The::engineController();
    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)), SLOT(update()) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)), SLOT(update()) );
}

bool
SimilarArtistsEngine::sourceRequestEvent( const QString &name )
{
    if( !name.startsWith( "similarArtists" ) )
        return false;

    bool force( false );
    QStringList tokens = name.split( QLatin1Char(':'), Qt::SkipEmptyParts );
    if( tokens.contains( QLatin1String("forceUpdate") ) )
        force = true;

    if( tokens.contains( QLatin1String("artist") ) )
        return update( m_artist );
    else
        return update( force );
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
        removeAllData( "similarArtists" );
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

bool
SimilarArtistsEngine::update( const QString &name )
{
    if( name.isEmpty() )
        return false;

    m_artist = name;
    similarArtistsRequest( m_artist );
    return true;
}

void
SimilarArtistsEngine::similarArtistsRequest( const QString &artistName )
{
    // we generate the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getSimilar" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", artistName );
    url.addQueryItem( "limit",  QString::number( m_maxArtists ) );

    The::networkAccessManager()->getData( url, this,
         SLOT(parseSimilarArtists(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
SimilarArtistsEngine::parseSimilarArtists( const QUrl &url, consr QByteArray &data,
                                           NetworkAccessManagerProxy::Error e )
{
    if( e.code != QNetworkReply::NoError )
    {
        removeAllData( "similarArtists" );
        warning() << "Failed to parse similar artists xml:" << url << e.description;
        return;
    }

    if( data.isEmpty() )
        return;

    QXmlStreamReader xml( data );
    SimilarArtist::List saList = SimilarArtist::listFromXml( xml );
    debug() << "Found" << saList.size() << "similar artists to" << m_artist;
    Plasma::DataEngine::Data eData;
    eData[ "artist"  ] = m_artist;
    eData[ "similar" ] = QVariant::fromValue( saList );
    setData( "similarArtists", eData );
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
}

#include "SimilarArtistsEngine.moc"
