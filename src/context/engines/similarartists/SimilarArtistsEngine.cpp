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

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h"

#include <QTimer>
#include <QXmlStreamReader>

K_EXPORT_AMAROK_DATAENGINE( similarArtists, SimilarArtistsEngine )

using namespace Context;

SimilarArtistsEngine::SimilarArtistsEngine( QObject *parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , m_maxArtists( 5 )
{
    EngineController *engine = The::engineController();
    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)), SLOT(update()) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)), SLOT(update()) );
}

SimilarArtistsEngine::~SimilarArtistsEngine()
{
}

bool
SimilarArtistsEngine::sourceRequestEvent( const QString &name )
{
    if( !name.startsWith( "similarArtists" ) )
        return false;

    bool force( false );
    QStringList tokens = name.split( QLatin1Char(':'), QString::SkipEmptyParts );
    if( tokens.contains( QLatin1String("forceUpdate") ) )
        force = true;

    if( tokens.contains( QLatin1String("artist") ) )
        update( m_artist );
    else
        update( force );
    return true;
}

void
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
    }
    else   //valid artist
    {
        // wee make a request only if the artist is different
        if( force || (newArtist != m_artist) )
        {
            // if the artist has changed
            m_artist = newArtist;
            similarArtistsRequest( m_artist );
        }
    }
}

void
SimilarArtistsEngine::update( const QString &name )
{
    if( name.isEmpty() )
        return;

    m_artist = name;
    similarArtistsRequest( m_artist );
}

void
SimilarArtistsEngine::similarArtistsRequest( const QString &artistName )
{
    // we generate the url for the demand on the lastFM Api
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getSimilar" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", artistName );
    url.addQueryItem( "limit",  QString::number( m_maxArtists ) );

    The::networkAccessManager()->getData( url, this,
         SLOT(parseSimilarArtists(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
SimilarArtistsEngine::parseSimilarArtists( const KUrl &url, QByteArray data,
                                           NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
    {
        removeAllData( "similarArtists" );
        debug() << "Error: failed to parse similar artists xml!" << e.description;
        return;
    }

    if( data.isEmpty() )
        return;

    QXmlStreamReader xml( data );
    xml.readNextStartElement(); // lfm
    if( xml.attributes().value(QLatin1String("status")) != QLatin1String("ok") )
    {
        removeAllData( "similarArtists" );
        return;
    }

    SimilarArtist::List saList;
    xml.readNextStartElement(); // similarartists
    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String("artist") )
        {
            QString name;
            KUrl artistUrl;
            KUrl imageUrl;
            float match( 0.0 );
            while( xml.readNextStartElement() )
            {
                const QStringRef &n = xml.name();
                const QXmlStreamAttributes &a = xml.attributes();
                if( n == QLatin1String("name") )
                    name = xml.readElementText();
                else if( n == QLatin1String("match") )
                    match = xml.readElementText().toFloat() * 100.0;
                else if( n == QLatin1String("url") )
                    artistUrl = KUrl( xml.readElementText() );
                else if( n == QLatin1String("image")
                         && a.hasAttribute(QLatin1String("size"))
                         && a.value(QLatin1String("size")) == QLatin1String("large") )
                    imageUrl = KUrl( xml.readElementText() );
                else
                    xml.skipCurrentElement();
            }
            SimilarArtistPtr artist( new SimilarArtist( name, match, artistUrl, imageUrl, m_artist ) );
            saList.append( artist );
        }
        else
            xml.skipCurrentElement();
    }

    debug() << "Found" << saList.size() << "similar artists of" << m_artist;
    Plasma::DataEngine::Data eData;
    eData[ "artist"  ] = m_artist;
    eData[ "similar" ] = qVariantFromValue( saList );
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
