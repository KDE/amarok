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
    , m_isDelayingSetData( false )
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
SimilarArtistsEngine::artistDescriptionRequest( const QString &artistName )
{
    // we genere the url for the demand on the lastFM Api
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getInfo" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", artistName );

    The::networkAccessManager()->getData( url, this,
         SLOT(parseArtistDescription(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
SimilarArtistsEngine::artistTopTrackRequest( const QString &artistName )
{
    // we genere the url for the demand on the lastFM Api
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.gettoptracks" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist",  artistName );

    The::networkAccessManager()->getData( url, this,
         SLOT(parseArtistTopTrack(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
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

    SimilarArtist::List saList;
    QXmlStreamReader xml( data );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == "artist" )
        {
            QString name;
            KUrl artistUrl;
            KUrl imageUrl;
            float match( 0.0 );
            while( !xml.atEnd() )
            {
                xml.readNext();
                const QStringRef &n = xml.name();
                if( xml.isEndElement() && n == "artist" )
                    break;
                if( !xml.isStartElement() )
                    continue;

                const QXmlStreamAttributes &a = xml.attributes();
                if( n == "name" )
                    name = xml.readElementText();
                else if( n == "match" )
                    match = xml.readElementText().toFloat() * 100.0;
                else if( n == "url" )
                    artistUrl = KUrl( xml.readElementText() );
                else if( n == "image" && a.hasAttribute("size") && a.value("size") == "large" )
                    imageUrl = KUrl( xml.readElementText() );
                else
                    xml.skipCurrentElement();
            }
            SimilarArtistPtr artist( new SimilarArtist( name, match, artistUrl, imageUrl, m_artist ) );
            saList.append( artist );
            artistDescriptionRequest( name );
            artistTopTrackRequest( name );
        }
    }

    debug() << QString( "Found %1 similar artists of '%2'" ).arg( saList.size() ).arg( m_artist );
    Plasma::DataEngine::Data eData;
    eData[ "artist"  ] = m_artist;
    eData[ "similar" ] = qVariantFromValue( saList );
    setData( "similarArtists", eData );
}

void
SimilarArtistsEngine::parseArtistDescription( const KUrl &url, QByteArray data,
                                              NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
        return;

    if( data.isEmpty() )
        return;

    QString name;
    QString summary;
    QXmlStreamReader xml( data );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == "artist" )
        {
            while( !xml.atEnd() )
            {
                xml.readNext();
                if( xml.isEndElement() && xml.name() == "artist" )
                    break;
                if( !xml.isStartElement() )
                    continue;

                if( xml.name() == "bio" )
                {
                    while( !xml.atEnd() )
                    {
                        xml.readNext();
                        if( xml.isEndElement() && xml.name() == "bio" )
                            break;
                        if( !xml.isStartElement() )
                            continue;

                        if( xml.name() == "summary" )
                            summary = xml.readElementText().simplified();
                        else
                            xml.skipCurrentElement();
                    }
                }
                else if( xml.name() == "name" )
                    name = xml.readElementText();
                else
                    xml.skipCurrentElement();
            }
        }
    }

    Plasma::DataEngine::Data eData;
    eData[ "artist" ] = name;
    eData[ "text"   ] = summary;
    m_descriptions << eData;
    if( !m_isDelayingSetData )
        delayedSetData();
}

void
SimilarArtistsEngine::parseArtistTopTrack( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
        return;

    if( data.isEmpty() )
        return;

    QString artist;
    QString topTrack;
    QXmlStreamReader xml( data );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == "track" )
        {
            while( !xml.atEnd() )
            {
                xml.readNext();
                if( xml.isEndElement() && xml.name() == "track" )
                    break;
                if( !xml.isStartElement() )
                    continue;

                if( xml.name() == "artist" )
                {
                    while( !xml.atEnd() )
                    {
                        xml.readNext();
                        if( xml.isEndElement() && xml.name() == "artist" )
                            break;
                        if( !xml.isStartElement() )
                            continue;

                        if( xml.name() == "name" )
                            artist = xml.readElementText();
                        else
                            xml.skipCurrentElement();
                    }
                }
                else if( xml.name() == "name" )
                    topTrack = xml.readElementText();
                else
                    xml.skipCurrentElement();
            }
        }

        if( !artist.isEmpty() && !topTrack.isEmpty() )
            break;
    }

    Plasma::DataEngine::Data eData;
    eData[ "artist" ] = artist;
    eData[ "track"  ] = topTrack;
    m_topTracks << eData;
    if( !m_isDelayingSetData )
        delayedSetData();
}

void
SimilarArtistsEngine::delayedSetData()
{
    m_isDelayingSetData = true;
    if( m_topTracks.isEmpty() && m_descriptions.isEmpty() )
    {
        m_isDelayingSetData = false;
    }
    else
    {
        if( !m_descriptions.isEmpty() )
            setData( "description", m_descriptions.takeFirst() );

        if( !m_topTracks.isEmpty() )
            setData( "toptrack", m_topTracks.takeFirst() );

        QTimer::singleShot( 50, this, SLOT(delayedSetData()) );
    }
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
