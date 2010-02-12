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
#include "SimilarArtistsEngine.h"

#include "Amarok.h"
#include <lastfm/Artist>
#include "ContextObserver.h"
#include "ContextView.h"
#include "Debug.h"
#include "EngineController.h"

#include <QDomElement>

using namespace Context;

 /**
  * Construct the engine
  * @param parent The object parent to this engine
  */
SimilarArtistsEngine::SimilarArtistsEngine( QObject *parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
        , m_similarArtistsJob( 0 )
        , m_currentSelection( "artist" )
        , m_requested( true )
        , m_sources( "current" )
        , m_triedRefinedSearch( 0 )
{
    update();
}

/**
 * Destroy the dataEngine
 */
SimilarArtistsEngine::~SimilarArtistsEngine()
{
    delete m_similarArtistsJob;
}

QMap<int, QString> SimilarArtistsEngine::similarArtists( const QString &artist_name )
{
    lastfm::Artist artist( artist_name );
    return artist.getSimilar( artist.getSimilar() );
}

QStringList SimilarArtistsEngine::sources() const
{
    return m_sources;
}

bool SimilarArtistsEngine::sourceRequestEvent( const QString &name )
{
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );

    // user has changed the maximum artists returned.
    if ( tokens.contains( "maxArtists" ) && tokens.size() > 1 )
        if (( tokens.at( 1 ) == QString( "maxArtists" ) )  && ( tokens.size() > 2 ) )
            m_maxArtists = tokens.at( 2 ).toInt();


    // otherwise, it comes from the engine, a new track is playing.
    removeAllData( name );
    setData( name, QVariant() );
    update();

    return true;
}

void SimilarArtistsEngine::message( const ContextState &state )
{
    if ( state == Current && m_requested )
        update();
}

void SimilarArtistsEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

    update();
}

/**
 * Prepare the calling of the similarArtistsRequest method.
 * Launch when the track played on amarok has changed.
 */
void SimilarArtistsEngine::update()
{
    //new update, if a job is not terminated, we kill it
    if ( m_similarArtistsJob )
    {
        m_similarArtistsJob->kill();
        m_similarArtistsJob = 0;
    }

    // we mark the jobs that fetch desciption as outdated
    m_descArtistJobs.clear();

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

        if ( artistName.compare( "" ) == 0 )   // Unknown artist
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
SimilarArtistsEngine::similarArtistsRequest( const QString &artist_name )
{
    // we genere the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getSimilar" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist", artist_name.toLocal8Bit() );
    url.addQueryItem( "limit",  QString::number( m_maxArtists ) );

    m_artist = artist_name;

    m_similarArtistsJob = KIO::storedGet( url,
                                          KIO::NoReload,
                                          KIO::HideProgressInfo );

    connect( m_similarArtistsJob,
             SIGNAL( result( KJob* ) ),
             SLOT( similarArtistsParse( KJob* ) ) );
}


/**
 * Fetches the description of the artist artist_name on the LastFM API.
 * @param artist_name the name of the artist
 */
void SimilarArtistsEngine::descArtistRequest(const QString& artist_name)
{    
    // we genere the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getinfo" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist", artist_name.toLocal8Bit() );
    url.addQueryItem( "lang", KGlobal::locale()->language() );


    KJob *job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );

    m_descArtistJobs.append(job);

    connect( job, SIGNAL( result( KJob* ) ), SLOT( descArtistParse( KJob* ) ) );

}


/**
 * Parse the xml fetched on the lastFM API.
 * Launched when the download of the data are finished.
 * @param job The job, which have downloaded the data.
 */
void
SimilarArtistsEngine::similarArtistsParse( KJob *job ) // SLOT
{
    // we clear the context of the dataEngine
    m_similarArtists.clear();   // we clear the similarArtists precedently downloaded
    m_descArtists=0;            // we mark we haven't downloaded the description of
                                // the artists

    if ( !m_similarArtistsJob ) return; //track changed while we were fetching

    // It's the correct job but it errored out
    if ( job->error() != KJob::NoError && job == m_similarArtistsJob )
    {
        // probably we haven't access to internet
        // sent a empty list
        QVariant variant( QMetaType::type( "SimilarArtist::SimilarArtistsList" )
                          , &m_similarArtists );
        setData( "similarArtists", "SimilarArtists", variant );
        m_similarArtistsJob = 0; // clear job
        return;
    }

    // not the right job, so let's ignore it
    if ( job != m_similarArtistsJob )
        return;

    if ( job )
    {
        KIO::StoredTransferJob* const storedJob
                                    = static_cast<KIO::StoredTransferJob*>( job );
                                    
        m_xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }
    else
    {
        return;
    }

    QDomDocument doc;
    doc.setContent( m_xml );

    const QDomNode events = doc.documentElement().namedItem( "similarartists" );

    QDomNode n = events.firstChild();
    while ( !n.isNull() )
    {
        // Similar artist name
        QDomNode nameNode = n.namedItem( "name" );
        QDomElement eName = nameNode.toElement();
        QString name;
        if ( !eName.isNull() )
        {
            name = eName.text();
        }

        // Match of the similar artist
        QDomNode matchNode = n.namedItem( "match" );
        QDomElement eMatch = matchNode.toElement();
        float match;
        if ( !eMatch.isNull() )
        {
            match = eMatch.text().toFloat(); //implicite cast

            //FIX of the lastFM API
            // this API return randomly a float between 0 to 1 of 0 to 100
            if( match<=1.0 ) {
                match=match*100.0;
            }
            
        }



        // Url of the similar artist on last.fm
        QDomNode urlNode = n.namedItem( "url" );
        QDomElement eUrl = urlNode.toElement();
        KUrl url;
        if ( !eUrl.isNull() )
        {
            url = KUrl( eUrl.text() );
        }


        // Url of the large similar artist image
        QDomNode imageUrlNode = n.namedItem( "image" );
        QDomElement imageUrlElement = imageUrlNode.toElement();
        KUrl imageUrl;
        if ( !imageUrlElement.isNull() )
        {
            while ( imageUrlElement.attributeNode( "size" ).value() != "large" )
            {
                imageUrlElement = imageUrlElement.nextSiblingElement();
            }
            imageUrl = KUrl( imageUrlElement.text() );
        }


        m_similarArtists.append( SimilarArtist( name, match, url, imageUrl, m_artist ) );
        descArtistRequest(name);
        
        n = n.nextSibling();
    }

    m_similarArtistsJob = 0;

}

 /**
  * Parse the xml fetched on the lastFM API for the similarArtist description
  * Launched when the download of the data are finished and for each similarArtists.
  * @param job The job, which have downloaded the data.
  */
void SimilarArtistsEngine::descArtistParse(KJob *job)
{
    DEBUG_BLOCK

    debug()<< "SAE 1";
    QString xml;
    int cpt=0;
    while (cpt>m_descArtistJobs.size() && m_descArtistJobs.at(cpt)!=job) {
        cpt++;
    }    
    
    if ( cpt>=m_descArtistJobs.size() ) return; //track changed while we were fetching

    // else (the job is correct)
    m_descArtists++;
    
    // It's the correct job but it errored out
    if ( job->error() != KJob::NoError)
    {
        return;
    }

    if ( job )
    {
        KIO::StoredTransferJob* const storedJob
                                    = static_cast<KIO::StoredTransferJob*>( job );

        xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }
    else
    {
        return;
    }

    QDomDocument doc;
    doc.setContent( xml );

    const QDomNode infos = doc.documentElement().namedItem( "artist" );
    
    QDomNode nameNode = infos.namedItem( "name" );
    QDomElement eName = nameNode.toElement();
    QString name;
    if ( !eName.isNull() )
    {
       name = eName.text();
    }

    QDomNode bioNode = infos.namedItem( "bio" );
    QDomElement eBio = bioNode.toElement();

    QDomNode descNode = eBio.namedItem( "summary" );
    QDomElement eDesc = descNode.toElement();
    QString desc;
    if ( !eDesc.isNull() )
    {
        desc = eDesc.text();
    }

    debug() << "SAE 2";
    // we search the correct artist to add his description
    cpt=0;
    while(cpt<m_similarArtists.size() && m_similarArtists.value(cpt).name()!=name)
    {
        cpt++;
    }

    if(cpt<m_similarArtists.size()) // we have find the correct artist
    {
        // we had his desciption
        SimilarArtist tmp=m_similarArtists.takeAt(cpt);
        tmp.setDesc(desc);
        m_similarArtists.insert(cpt,tmp);
    }    

    if(m_descArtists+1>=m_similarArtists.size())  // we have fetched all of the data
    {
        // we send the data to the applet
        QVariant variant( QMetaType::type( "SimilarArtist::SimilarArtistsList" )
                      , &m_similarArtists );
        setData( "similarArtists", "SimilarArtists", variant );
    }
}


#include "SimilarArtistsEngine.moc"