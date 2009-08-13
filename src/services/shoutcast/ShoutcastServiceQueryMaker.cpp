/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2007 Adam Pigg <adam@piggz.co.uk>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ShoutcastServiceQueryMaker.h"

#include "Amarok.h"
#include "Debug.h"
#include "ServiceMetaBase.h"
#include "ShoutcastMeta.h"
#include "collection/support/MemoryMatcher.h"

#include <kio/job.h>

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

 #include <QDomDocument>

using namespace Meta;

struct ShoutcastServiceQueryMaker::Private {
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    QueryType type;
    int maxsize;
    bool returnDataPtrs;
};


ShoutcastServiceQueryMaker::ShoutcastServiceQueryMaker( ShoutcastServiceCollection * collection, bool isTop500Query )
 : DynamicServiceQueryMaker()
 , m_storedTransferJob( 0 )
 , d( new Private )
{
    DEBUG_BLOCK

    m_collection = collection;
    m_top500 = isTop500Query;
    reset();
}

ShoutcastServiceQueryMaker::~ShoutcastServiceQueryMaker()
{
    DEBUG_BLOCK

    delete d;
}

QueryMaker * ShoutcastServiceQueryMaker::reset()
{
    DEBUG_BLOCK;
    
    d->type = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    m_genreMatch.clear();
    m_filter.clear();

    return this;
}

QueryMaker*
ShoutcastServiceQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

void ShoutcastServiceQueryMaker::run()
{
    DEBUG_BLOCK

    if ( d->type == Private::NONE )
        //TODO error handling
        return;
    else if ( m_top500 )
        fetchTop500();
    else if (  d->type == Private::GENRE )
        fetchGenres();
    else if (  d->type == Private::TRACK )
        fetchStations();
}

void ShoutcastServiceQueryMaker::runQuery()
{
    DEBUG_BLOCK

    if ( m_storedTransferJob )
        return;

    m_collection->acquireReadLock();
    //naive implementation, fix this
    //note: we are not handling filtering yet

    //this is where the fun stuff happens
    if ( m_top500 )
        fetchTop500();
    else if (  d->type == Private::GENRE )
        fetchGenres();
    else if (  d->type == Private::TRACK )
        fetchStations();

    m_collection->releaseLock();
}


void ShoutcastServiceQueryMaker::abortQuery()
{}

QueryMaker * ShoutcastServiceQueryMaker::setQueryType( QueryType type )
{
    DEBUG_BLOCK

    switch( type )
    {
        case QueryMaker::Track:
            d->type = Private::TRACK;
            return this;

        case QueryMaker::Genre:
            d->type = Private::GENRE;
            return this;

        case QueryMaker::Artist:
        case QueryMaker::Album:
        case QueryMaker::Composer:
        case QueryMaker::Year:
        case QueryMaker::Custom:
        case QueryMaker::None:
            return this;
    }

    return this;
}

QueryMaker * ShoutcastServiceQueryMaker::addMatch(const Meta::GenrePtr & genre)
{
    DEBUG_BLOCK

    m_genreMatch = genre->name();
    return this;
}

template<class PointerType, class ListType>
void ShoutcastServiceQueryMaker::emitProperResult( const ListType& list )
{
    DEBUG_BLOCK

    if ( d->returnDataPtrs )
    {
        DataList data;
        foreach( PointerType p, list )
            data << DataPtr::staticCast( p );

        emit newResultReady( m_collection->collectionId(), data );
    }
    else
        emit newResultReady( m_collection->collectionId(), list );
}

void ShoutcastServiceQueryMaker::handleResult()
{
    DEBUG_BLOCK

    switch( d->type )
    {
        case Private::GENRE :
        {
            GenreList genres = m_collection->genreMap().values();
            if ( d->maxsize >= 0 && genres.count() > d->maxsize )
                genres = genres.mid( 0, d->maxsize );
            emitProperResult<GenrePtr, GenreList>( genres );
            break;
        }
        case Private::TRACK :
        {
            TrackList tracks = m_currentTrackQueryResults;
            if ( d->maxsize >= 0 && tracks.count() > d->maxsize )
                tracks = tracks.mid( 0, d->maxsize );
            emitProperResult<TrackPtr, TrackList>( tracks );
            break;
        }

        default:
            warning() << "Query type not handled.";
    }
}

void ShoutcastServiceQueryMaker::fetchGenres()
{
    DEBUG_BLOCK
    //check if we already have the genres
    /*if ( m_collection->genreMap().values().count() != 0 && m_filter.isEmpty() )
    {
        handleResult();
        debug() << "no need to fetch genres again! ";
    }
    else*/ if ( m_filter.isEmpty() )
    {
        
        m_collection->acquireReadLock();
        m_collection->setGenreMap( GenreMap() );
        m_collection->setTrackMap( TrackMap() );
        m_collection->releaseLock();
        
        m_storedTransferJob =  KIO::storedGet(  KUrl( "http://www.shoutcast.com/sbin/newxml.phtml" ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) ), this, SLOT( genreDownloadComplete(KJob *) ) );
    } else {

        m_collection->acquireReadLock();
        m_collection->setGenreMap( GenreMap() );
        m_collection->setTrackMap( TrackMap() );
        m_collection->releaseLock();
        
        ServiceGenre * genre = new ServiceGenre( i18n( "Results for: %1", m_filter ) );
        GenrePtr genrePtr( genre );
        m_collection->acquireWriteLock();
        m_collection->addGenre( genrePtr );
        m_collection->releaseLock();
        
        handleResult();
        emit( queryDone() );
    }
}


void ShoutcastServiceQueryMaker::fetchStations()
{
    DEBUG_BLOCK

    bool refetch = false;

    if ( m_collection->genreMap().isEmpty() )
        refetch = true;
    else {
        GenreMatcher genreMatcher( m_collection->genreMap()[m_genreMatch] );
        m_currentTrackQueryResults = genreMatcher.match( m_collection );
        if ( m_currentTrackQueryResults.count() == 0 )
            refetch = true;
    }

    
    if( !refetch && m_filter.isEmpty() )
    {
        handleResult();
    }
    else if ( m_filter.isEmpty() )
    {
        m_storedTransferJob =  KIO::storedGet( "http://www.shoutcast.com/sbin/newxml.phtml?genre=" + m_genreMatch, KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) ), this, SLOT( stationDownloadComplete(KJob *) ) );
    } else {

        debug() << "fetching tracks with filter: " << m_filter << " url: " << "http://www.shoutcast.com/sbin/newxml.phtml?genre=&s=" + m_filter;
        m_storedTransferJob =  KIO::storedGet( "http://www.shoutcast.com/sbin/newxml.phtml?search=" + m_filter, KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) ), this, SLOT( stationDownloadComplete(KJob *) ) );

    }
}

void ShoutcastServiceQueryMaker::fetchTop500()
{
    DEBUG_BLOCK

    if ( m_filter.isEmpty() )
    {
        m_storedTransferJob =  KIO::storedGet( KUrl ( "http://www.shoutcast.com/sbin/newxml.phtml?genre=Top500" ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) ), this, SLOT( stationDownloadComplete(KJob *) ) );
    }
    else
    {
        debug() << "fetching tracks with filter: " << m_filter << " url: " << "http://www.shoutcast.com/sbin/newxml.phtml?genre=Top500&search=" + m_filter;
        m_storedTransferJob =  KIO::storedGet( KUrl ( "http://www.shoutcast.com/sbin/newxml.phtml?genre=Top500&search=" + m_filter ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) ), this, SLOT( stationDownloadComplete(KJob *) ) );
    }
}


void ShoutcastServiceQueryMaker::genreDownloadComplete(KJob * job)
{
    DEBUG_BLOCK
    
    if ( job->error() )
    {
        error() << job->error();
        error() << job->errorString();
        m_storedTransferJob->deleteLater();
        return;
    }

    QDomDocument doc( "genres" );

    doc.setContent( m_storedTransferJob->data() );

    //debug() << "So far so good... Got this data: " << m_storedTransferJob->data();


    // We use this list to filter out some obscure genres
    QStringList bannedGenres;
    bannedGenres << "aaa" << "alles" << "any" << "anything" << "autopilot" << "backup" << "bandas" << "beer";
    bannedGenres << "catholic" << "chr" << "das" << "domaca" << "everything" << "fire" << "her" << "hollands";
    bannedGenres << "http" << "just" << "lokale" << "middle" << "noticias" << "only" << "scanner" << "shqip";
    bannedGenres << "good" << "super" << "wusf" << "www" << "zabavna" << "zouk" << "whatever" << "varios";
    bannedGenres << "varius" << "video" << "opm" << "non" << "narodna" << "muzyka" << "muzica" << "muzika";
    bannedGenres << "musique" << "music" << "multi" << "online" << "mpb" << "musica" << "musik" << "manele";
    bannedGenres << "paranormal" << "todos" << "soca" << "the" << "toda" << "trova" << "italo";
    bannedGenres << "auto" << "alternativo" << "best" << "clasicos" << "der" << "desi" << "die" << "emisora";
    bannedGenres << "voor" << "post" << "playlist" << "ned" << "gramy" << "deportes" << "bhangra" << "exitos";
    bannedGenres << "doowop" << "radio" << "radyo" << "railroad" << "program" << "mostly" << "hot";
    bannedGenres << "deejay" << "cool" << "big" << "exitos" << "mp3" << "muzyczne" << "nederlandstalig";
    bannedGenres << "max" << "informaci" << "halk" << "dobra" << "welcome" << "genre" << "nederlands";
    bannedGenres << "nederlandse" << "hrvatska" << "populara" << "universidad";


    GenreMap genreMap;

    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        const QString name = e.attribute( "name" );
        if( !name.isNull() && !bannedGenres.contains( name.toLower() ) && !genreMap.contains( name ) )
        {

           // debug() << "add genre: " << name;

            ServiceGenre * genre = new ServiceGenre( name );
            GenrePtr genrePtr( genre );
            m_collection->acquireWriteLock();
            m_collection->addGenre( genrePtr );
            m_collection->releaseLock();

        }
        n = n.nextSibling();
    }

    m_storedTransferJob->deleteLater();

    handleResult();
    emit queryDone();
}

void ShoutcastServiceQueryMaker::stationDownloadComplete( KJob *job )
{
    DEBUG_BLOCK

    if ( job->error() )
    {
        error() << job->error();
        m_storedTransferJob->deleteLater();
        return;
    }

    m_currentTrackQueryResults.clear();

    QDomDocument doc( "list" );

    doc.setContent( m_storedTransferJob->data() );

    //Go through the XML file and add all the stations
    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if( e.hasAttribute( "name" ) )
        {
            if( !e.attribute( "name" ).isNull() /*&& ! m_currentTrackQueryResults.contains( e.attribute( "name" ) )*/ )
            {

                QString name = "(" + e.attribute( "lc" ) + ") " +  e.attribute( "name" ); //lc: listeners count

                QString playlistUrl = "http://www.shoutcast.com/sbin/shoutcast-playlist.pls?rn="
                        + e.attribute( "id" ) + "&file=filename.pls";

                ShoutcastTrack * track = new ShoutcastTrack(  name, playlistUrl );

                TrackPtr trackPtr( track );
                m_collection->acquireWriteLock();
                m_collection->addTrack( trackPtr );
                m_collection->releaseLock();

                if ( m_top500 ){
                    // TODO: Handle filtering when top500 stations are requested...
                    if ( m_filter.isEmpty() ){
                    }else{
                    }
                }else{
                    if ( m_filter.isEmpty() ) {
                        GenrePtr genrePtr = m_collection->genreMap()[ m_genreMatch ];
                        ServiceGenre * genre = static_cast<  ServiceGenre * >( genrePtr.data() );
                        genre->addTrack( trackPtr );
                        track->setGenre( genrePtr );
                    } else {
                        GenrePtr genrePtr = m_collection->genreMap()[ "Results for: " + m_filter ];
                        ServiceGenre * genre = static_cast<  ServiceGenre * >( genrePtr.data() );

                        if ( genre == 0 )  // sanity check as this has been reported to cause crashes
                            return;

                        genre->addTrack( trackPtr );
                        track->setGenre( genrePtr );
                    }
                }

                m_currentTrackQueryResults.push_front( trackPtr );

            }
        }
        n = n.nextSibling();
    }

    m_storedTransferJob->deleteLater();
    handleResult();
    emit queryDone();
}

QueryMaker * ShoutcastServiceQueryMaker::addFilter(qint64 value, const QString & filter, bool matchBegin, bool matchEnd)
{
    DEBUG_BLOCK

    Q_UNUSED( matchBegin ); Q_UNUSED( matchEnd );

    //debug() << "value: " << value;
    //for now, only accept artist filters
    if ( value == valGenre ) {

       /* if ( d->type == Private::GENRE ) {
            m_collection->acquireReadLock();
            m_collection->setGenreMap( GenreMap() );
            m_collection->setTrackMap( TrackMap() );
            m_collection->releaseLock();
        }*/

        debug() << "Filter: " << filter;
        m_filter = filter;
    }

    return this;
}


#include "ShoutcastServiceQueryMaker.moc"


