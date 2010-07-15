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
#include "ContextView.h"
#include "EngineController.h"

#include <lastfm/Artist>

#include <KConfigGroup>

#include <QTimer>
#include <QXmlStreamReader>

K_EXPORT_AMAROK_DATAENGINE( similarArtists, SimilarArtistsEngine )

using namespace Context;

SimilarArtistsEngine::SimilarArtistsEngine( QObject *parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , Engine::EngineObserver( The::engineController() )
    , m_isDelayingSetData( false )
{
    m_descriptionWideLang = "aut";
}

SimilarArtistsEngine::~SimilarArtistsEngine()
{
}

QMap<int, QString>
SimilarArtistsEngine::similarArtists( const QString &artistName )
{
    lastfm::Artist artist( artistName );
    return artist.getSimilar( artist.getSimilar() );
}

bool
SimilarArtistsEngine::sourceRequestEvent( const QString &name )
{
    if( name == "similarArtists" )
    {
        m_currentTrack = The::engineController()->currentTrack();
    }
    else
    {
        QStringList tokens = name.split( ':' );
        if( tokens.contains( "maxArtists" ) && tokens.size() > 1 )
        {
            // user has changed the maximum artists returned.
            if( ( tokens.at( 1 ) == QString( "maxArtists" ) )  && ( tokens.size() > 2 ) )
                m_maxArtists = tokens.at( 2 ).toInt();
        }
        else if( tokens.contains( "lang" ) && tokens.size() > 1 )
        {
            // user has selected is favorite language.
            if( ( tokens.at( 1 ) == QString( "lang" ) )  && ( tokens.size() > 2 ) )
                m_descriptionWideLang = tokens.at( 2 );
        }
    }
    update();
    return true;
}

void
SimilarArtistsEngine::metadataChanged( Meta::TrackPtr track )
{
    if( m_currentTrack->artist() != track->artist() )
        update();
}

void
SimilarArtistsEngine::engineNewTrackPlaying()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
    {
        removeAllData( "similarArtists" );
    }
    else if( !m_currentTrack )
    {
        m_currentTrack = track;
        subscribeTo( track );
        update();
    }
    else if( m_currentTrack != track )
    {
        Meta::TrackPtr oldTrack = m_currentTrack;
        unsubscribeFrom( m_currentTrack );
        m_currentTrack = track;
        subscribeTo( track );
        if( m_currentTrack->artist() != oldTrack->artist() )
            update();
    }
}

void
SimilarArtistsEngine::update()
{
    if( !m_currentTrack )
        return;

    QString artistName;
    if( m_currentTrack->artist() )
    {
        if (( m_currentTrack->playableUrl().protocol() == "lastfm" ) ||
            ( m_currentTrack->playableUrl().protocol() == "daap" ) ||
            !The::engineController()->isStream() )
            artistName = m_currentTrack->artist()->name();
        else
            artistName = m_currentTrack->artist()->prettyName();
    }

    if( artistName.isEmpty() )   // Unknown artist
    {
        m_artist = "Unknown artist";
        removeAllData( "similarArtists" );
    }
    else   //valid artist
    {
        // Read config and inform the engine.
        KConfigGroup config = Amarok::config( "SimilarArtists Applet" );

        //fix the limit of the request, the default is already fixed by the applet
        int nbArt = config.readEntry( "maxArtists", "5" ).toInt();

        // wee make a request only if the artist is different
        // or if the number of artist to display is bigger
        if( artistName != m_artist || nbArt > m_maxArtists )   // we update the data only
        {
            // if the artist has changed
            m_maxArtists = nbArt;
            m_artist = artistName;
            similarArtistsRequest( artistName );
        }
    }
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
    url.addQueryItem( "method", "artist.getinfo" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", artistName );
    url.addQueryItem( "lang", descriptionLocale() );

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

    // The reader on the xml document which contains the information of the lastFM API.
    QXmlStreamReader xmlReader;

    if( !data.isEmpty() )
    {
        // we add to the reader the xml downloaded from lastFM
        xmlReader.addData( data );
    }
    else
    {
        return;
    }

    SimilarArtist::List saList;
    // we search the artist name on the xml
    while ( !xmlReader.atEnd() && !xmlReader.hasError() )
    {
        if ( xmlReader.name() == "artist" ) // we have found a similar artist
        {

            // we search the similar artist name
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.name() != "name" )
            {
                xmlReader.readNext();
            }

            QString name;
            // we get the name only if we have found it
            if ( !xmlReader.atEnd() && !xmlReader.hasError() )
            {
                name = xmlReader.readElementText();
            }


            // we search the similar artist match
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.name() != "match" )
            {
                xmlReader.readNext();
            }

            float match(0.0);
            // we get the match only if we have found it
            if ( !xmlReader.atEnd() && !xmlReader.hasError() )
            {
                match = xmlReader.readElementText().toFloat();

                //FIX of the lastFM API
                // this API return randomly a float between 0 to 1 of 0 to 100
                if ( match <= 1.0 )
                {
                    match = match * 100.0;
                }
            }

            // we search the url on lastFM of the similar artist
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.name() != "url" )
            {
                xmlReader.readNext();
            }

            KUrl url;
            // we get the url only if we have found it
            if ( !xmlReader.atEnd() && !xmlReader.hasError() )
            {
                url = KUrl( xmlReader.readElementText() );
            }


            // we search the url on lastFM of the artist image
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.name() != "image" )
            {
                xmlReader.readNext();
            }

            //we search the large image, in the panel of the image proposed by lastFM
            while ( !xmlReader.atEnd()
                    && !xmlReader.hasError()
                    && xmlReader.attributes().value( "size" ) != "large" )
            {
                xmlReader.readNext();
            }

            KUrl imageUrl;
            // we get the image url only if we have found it
            if ( !xmlReader.atEnd() && !xmlReader.hasError() )
            {
                imageUrl = KUrl( xmlReader.readElementText() );
            }

            if( !name.isEmpty() )
            {
                SimilarArtistPtr artist( new SimilarArtist( name, match, url, imageUrl, m_artist ) );
                saList.append( artist );
                artistDescriptionRequest( name );
                artistTopTrackRequest( name );
            }
        }
        xmlReader.readNext();
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

    // The reader on the xml document which contains the information of the lastFM API.
    QXmlStreamReader xmlReader;

    if( !data.isEmpty() )
    {
        // we add to the reader the xml downloaded from lastFM
        xmlReader.addData( data );
    }
    else
    {
        return;
    }

    // we search the artist name on the xml
    while ( !xmlReader.atEnd() && !xmlReader.hasError()  && xmlReader.name() != "name" )
    {
        xmlReader.readNext();
    }

    QString name;
    // we get the name only if we have found it
    if ( !xmlReader.atEnd() && !xmlReader.hasError() )
    {
        name = xmlReader.readElementText();
    }
    else   // error when parsing the xml
    {
        return;
    }

    // we search the artist description on the xml
    while ( !xmlReader.atEnd() && !xmlReader.hasError()  && xmlReader.name() != "summary" )
    {
        xmlReader.readNext();
    }

    QString description;
    // we get the description only if we have found it
    if ( !xmlReader.atEnd() && !xmlReader.hasError() )
    {
        description = xmlReader.readElementText().simplified(); //we clean the string
    }
    else
    {
        return;
    }

    Plasma::DataEngine::Data eData;
    eData[ "artist" ] = name;
    eData[ "text"   ] = description;
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

    // The reader on the xml document which contains the information of the lastFM API.
    QXmlStreamReader xmlReader;

    if( !data.isEmpty() )
    {
        // we add to the reader the xml downloaded from lastFM
        xmlReader.addData( data );
    }
    else
    {
        return;
    }

    // we search the name of the artist
    while ( !xmlReader.atEnd() && !xmlReader.hasError()  && xmlReader.name() != "toptracks" )
    {
        xmlReader.readNext();
    }

    //the name of the artist is on the xml attribute
    QString name;
    if ( !xmlReader.atEnd() && !xmlReader.hasError() )
    {
        name = xmlReader.attributes().value( "artist" ).toString();
    }
    else
    {
        return;
    }

    // we search the first top track on the xml
    while ( !xmlReader.atEnd() && !xmlReader.hasError()  && xmlReader.name() != "name" )
    {
        xmlReader.readNext();
    }

    QString topTrack;
    // we get the name only if we have found it
    if ( !xmlReader.atEnd() && !xmlReader.hasError() )
    {
        topTrack = xmlReader.readElementText();
    }
    else   // error when parsing the xml
    {
        return;
    }

    Plasma::DataEngine::Data eData;
    eData[ "artist" ] = name;
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

inline QString
SimilarArtistsEngine::descriptionLocale() const
{
    // if there is no language set (QLocale::C) then return english as default
    if ( m_descriptionWideLang == "aut" )
    {
        if ( m_descriptionLang.language() == QLocale::C )
            return "en";
        else
            return m_descriptionLang.name().split( '_' )[0];
    }
    else
        return m_descriptionWideLang;
}

#include "SimilarArtistsEngine.moc"
