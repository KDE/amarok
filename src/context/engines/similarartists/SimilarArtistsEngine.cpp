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

#include <QXmlStreamReader>

K_EXPORT_AMAROK_DATAENGINE( similarArtists, SimilarArtistsEngine )

using namespace Context;

/**
 * Construct the engine
 * @param parent The object parent to this engine
 */
SimilarArtistsEngine::SimilarArtistsEngine( QObject *parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
{
    m_descriptionWideLang="aut";
    m_currentSelection="artist";
    m_requested=true;
    m_sources.append("current");
    m_triedRefinedSearch=0;
    update();
}

/**
 * Destroy the dataEngine
 */
SimilarArtistsEngine::~SimilarArtistsEngine()
{
}

QMap<int, QString>
SimilarArtistsEngine::similarArtists( const QString &artistName )
{
    lastfm::Artist artist( artistName );
    return artist.getSimilar( artist.getSimilar() );
}

QStringList
SimilarArtistsEngine::sources() const
{
    return m_sources;
}

bool
SimilarArtistsEngine::sourceRequestEvent( const QString &name )
{
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );

    // user has changed the maximum artists returned.
    if ( tokens.contains( "maxArtists" ) && tokens.size() > 1 )
    {
        if (( tokens.at( 1 ) == QString( "maxArtists" ) )  && ( tokens.size() > 2 ) )
        {
            m_maxArtists = tokens.at( 2 ).toInt();
        }
    }
    // user has selected is favorite language.
    if ( tokens.contains( "lang" ) && tokens.size() > 1 )
    {
        if (( tokens.at( 1 ) == QString( "lang" ) )  && ( tokens.size() > 2 ) )
        {
            m_descriptionWideLang = tokens.at( 2 );
        }
    }

    // otherwise, it comes from the engine, a new track is playing.
    removeAllData( name );
    setData( name, QVariant() );
    update();

    return true;
}

void
SimilarArtistsEngine::message( const ContextState &state )
{
    if ( state == Current && m_requested )
        update();
}

void
SimilarArtistsEngine::metadataChanged( Meta::TrackPtr track )
{
    if( m_currentTrack->artist() != track->artist() )
        update();
}

/**
 * Prepare the calling of the similarArtistsRequest method.
 * Launch when the track played on amarok has changed.
 */
void
SimilarArtistsEngine::update()
{
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();

    // We've got a new track, great, let's fetch some info from SimilarArtists !
    m_triedRefinedSearch = 0;
    QString artistName;

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;

    DataEngine::Data data;
    // default, or applet told us to fetch artist
    if ( selection() == "artist" )
    {
        if ( currentTrack->artist() )
        {
            if (( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                     ( currentTrack->playableUrl().protocol() == "daap" ) ||
                     !The::engineController()->isStream() )
                artistName = currentTrack->artist()->name();
            else
                artistName = currentTrack->artist()->prettyName();
        }

        // we delete the previous update only if the artist requested was not the same
        if( artistName != m_artist ) {
            // new update, if a job is not terminated, we kill it
            // TODO: how to do this?
        }

        if ( artistName.isEmpty() )   // Unknown artist
        {
            m_artist = "Unknown artist";
            setData( "similarArtists", "artist", m_artist );

            // we send an empty list
            m_similarArtists.clear();
            QVariant variant( QMetaType::type( "SimilarArtist::SimilarArtistsList" ), &m_similarArtists );
            setData( "similarArtists", "SimilarArtists", variant );
        }
        else   //valid artist
        {
            // Read config and inform the engine.
            KConfigGroup config = Amarok::config( "SimilarArtists Applet" );

            //fix the limit of the request, the default is already fixed by the applet
            int nbArt = config.readEntry( "maxArtists", "5" ).toInt();

            // wee make a request only if the artist is different
            // or if the number of artist to display is bigger
            if ( artistName != m_artist || nbArt > m_maxArtists )   // we update the data only
            {
                // if the artist has changed
                m_maxArtists = nbArt;
                m_artist = artistName;
                setData( "similarArtists", "artist", artistName );
                similarArtistsRequest( artistName );
            }
        }
    }

}

/**
 * Fetches the similar artists for an artist thanks to the LastFm WebService
 * Store this in the similar artist list of this class
 * @param artist_name the name of the artist
 */
void
SimilarArtistsEngine::similarArtistsRequest( const QString &artistName )
{
    // we clear the context of the dataEngine
    m_similarArtists.clear();   // we clear the similarArtists precedently downloaded
    m_descriptionArtists = 0;   // we mark we haven't downloaded the description of the artists
    m_topTrackArtists = 0;      // we mark we haven't downloaded the most know tracks of the artists

    // we generate the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getSimilar" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist", artistName.toLocal8Bit() );
    url.addQueryItem( "limit",  QString::number( m_maxArtists ) );

    m_similarArtistsUrl = url;
    The::networkAccessManager()->getData( m_similarArtistsUrl, this,
         SLOT(parseSimilarArtists(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}


/**
 * Fetches the description of the artist artist_name on the LastFM API.
 * @param artist_name the name of the artist
 */
void
SimilarArtistsEngine::artistDescriptionRequest( const QString &artistName )
{
    // we genere the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getinfo" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist", artistName.toLocal8Bit() );
    url.addQueryItem( "lang", descriptionLocale() );

    m_artistDescriptionUrls << url;
    The::networkAccessManager()->getData( url, this,
         SLOT(parseArtistDescription(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}


/**
 * Fetches the the most known artist track of the artist artistName on the LastFM API
 * @param artistName the name of the artist
 */
void
SimilarArtistsEngine::artistTopTrackRequest( const QString &artistName )
{
    // we genere the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.gettoptracks" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist",  artistName.toLocal8Bit() );

    m_artistTopTrackUrls << url;
    The::networkAccessManager()->getData( url, this,
         SLOT(parseArtistTopTrack(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}


/**
 * Parse the xml fetched on the lastFM API.
 * Launched when the download of the data are finished.
 */
void
SimilarArtistsEngine::parseSimilarArtists( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e ) // SLOT
{
    if( !url.isValid() || m_similarArtistsUrl != url )
        return;

    m_similarArtistsUrl.clear();
    if( e.code != QNetworkReply::NoError )
    {
        // probably we haven't access to internet sent a empty list
        QVariant variant( QMetaType::type( "SimilarArtist::SimilarArtistsList" ), &m_similarArtists );
        m_similarArtistsUrl.clear();
        setData( "similarArtists", "SimilarArtists", variant );
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
        m_similarArtistsUrl.clear();
        return;
    }

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
                m_similarArtists.append( SimilarArtist( name, match, url, imageUrl, m_artist ) );
                artistDescriptionRequest( name );
                artistTopTrackRequest( name );
            }
        }
        xmlReader.readNext();
    }
    debug() << QString( "Found %1 similar artists of '%2'" ).arg( m_similarArtists.size() ).arg( m_artist );
}

/**
 * Parse the xml fetched on the lastFM API for the similarArtist description
 * Launched when the download of the data are finished and for each similarArtists.
 */
void
SimilarArtistsEngine::parseArtistDescription( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !url.isValid() || !m_artistDescriptionUrls.contains( url ) )
        return;

    m_artistDescriptionUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
        return;

    m_descriptionArtists++;

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

    // we search the correct artist to add his/her/their/its description
    QList<SimilarArtist>::iterator it;
    QList<SimilarArtist>::iterator endit = m_similarArtists.end();
    for( it = m_similarArtists.begin(); it != endit; ++it )
    {
        if( it->name() == name )
        {
            it->setDescription( description );
            break;
        }
    }

    // we have fetched all of the data (artists + descriptions + toptracks)
    if ( m_descriptionArtists + 1 >= m_similarArtists.size()
            && m_topTrackArtists + 1 >= m_similarArtists.size() )
    {
        // we send the data to the applet
        QVariant variant( QMetaType::type( "SimilarArtist::SimilarArtistsList" ) , &m_similarArtists );
        setData( "similarArtists", "SimilarArtists", variant );
    }
}

/**
 * Parse the xml fetched on the lastFM API for the similarArtist most known track
 * Launched when the download of the data are finished and for each similarArtists.
 */
void
SimilarArtistsEngine::parseArtistTopTrack( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_artistTopTrackUrls.contains( url ) )
        return;

    m_artistTopTrackUrls.remove( url );
    if( e.code != QNetworkReply::NoError )
        return;

    m_topTrackArtists++;

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

    // we search the correct artist to add his/her/their/its description
    QList<SimilarArtist>::iterator it;
    QList<SimilarArtist>::iterator endit = m_similarArtists.end();
    for( it = m_similarArtists.begin(); it != endit; ++it )
    {
        if( it->name() == name )
        {
            it->setTopTrack( topTrack );
            break;
        }
    }

    // we have fetched all of the data (artists + descriptions + toptracks)
    if ( m_descriptionArtists + 1 >= m_similarArtists.size()
            && m_topTrackArtists + 1 >= m_similarArtists.size() )
    {
        // we send the data to the applet
        QVariant variant( QMetaType::type( "SimilarArtist::SimilarArtistsList" ), &m_similarArtists );
        setData( "similarArtists", "SimilarArtists", variant );
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
