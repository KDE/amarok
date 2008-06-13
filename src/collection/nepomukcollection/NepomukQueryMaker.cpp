/*
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>
   Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukQueryMaker.h"
#include "NepomukCollection.h"
#include "NepomukMeta.h"

#include "Debug.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Soprano/Client/DBusClient>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/XMLSchema>

using namespace Meta;

class NepomukWorkerThread : public ThreadWeaver::Job
{
    public:
        NepomukWorkerThread( NepomukQueryMaker *queryMaker )
            : ThreadWeaver::Job()
            , m_queryMaker( queryMaker )
            , m_aborted( false )
        {
            //nothing to do
        }

        virtual void requestAbort()
        {
            m_aborted = true;
        }

    protected:
        virtual void run()
        {
            QString query = m_queryMaker->buildQuery();
            debug() << "Query:" << query << endl;
            if( !m_aborted )
                m_queryMaker->doQuery(query);
            setFinished( !m_aborted );
        }

    private:
        NepomukQueryMaker *m_queryMaker;
        bool m_aborted;
};



NepomukQueryMaker::NepomukQueryMaker(NepomukCollection *collection,Soprano::Client::DBusClient *client )
    : QueryMaker() 
    , m_collection(collection)

{
    worker = 0;
    this->client = client;
    reset();
}

NepomukQueryMaker::~NepomukQueryMaker()
{
    
}

QueryMaker*
NepomukQueryMaker::reset()
{
    queryType = NONE;
    queryMatch.clear();
    if( worker && worker->isFinished() )
        delete worker;   //TODO error handling
    return this;
}

void
NepomukQueryMaker::abortQuery()
{
}

void
NepomukQueryMaker::run()
{
    debug() << "run()" << endl;
    if( queryType == NONE )
            return; //better error handling?
    if( worker && !worker->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait or job to complete
    }
    else
    {
        worker = new NepomukWorkerThread(this);
        connect( worker, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( done( ThreadWeaver::Job* ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( worker );
    }
}

QueryMaker*
NepomukQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    debug() << "returnResultAsDataPtrs()" << resultAsDataPtrs << endl;
    resultAsDataPtrs = resultAsDataPtrs;
    return this;
}



QueryMaker*
NepomukQueryMaker::startTrackQuery()
{
    debug() << "startTrackQuery()" << endl;
    queryType  = TRACK;
    return this;
}

QueryMaker*
NepomukQueryMaker::startArtistQuery()
{
    queryType = ARTIST;
    debug() << "startArtistQuery()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::startAlbumQuery()
{
    queryType = ALBUM;
    debug() << "starAlbumQuery()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::startGenreQuery()
{
    debug() << "startGenreQuery()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::startComposerQuery()
{
    debug() << "startComposerQuery()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::startYearQuery()
{
    debug() << "startYearQuery()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::startCustomQuery()
{
    debug() << "startCustomQuery()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::includeCollection( const QString &collectionId )
{
    debug() << "includeCollection()" << endl;
	Q_UNUSED( collectionId )
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeCollection( const QString &collectionId )
{
    debug() << "excludeCollection()" << endl;
	Q_UNUSED( collectionId )
	return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const TrackPtr &track )
{
    debug() << "addMatch(Track)" << endl;
	Q_UNUSED( track )
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const ArtistPtr &artist )
{
    debug() << "addMatch(artist)" << endl;
    queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( Soprano::Vocabulary::Xesam::artist().toString() )
            .arg( artist->name() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );

    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const AlbumPtr &album )
{
    debug() << "addMatch(album)" << endl;
	
    queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( Soprano::Vocabulary::Xesam::album().toString() )
            .arg( album->name() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const GenrePtr &genre )
{
    debug() << "addMatch(genre)" << endl;
	Q_UNUSED( genre )
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const ComposerPtr &composer )
{
    debug() << "addMatch(composer)" << endl;
	Q_UNUSED( composer )
	return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const YearPtr &year )
{
    debug() << "addMatch(year)" << endl;
	Q_UNUSED( year )
	return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const DataPtr &data )
{
    debug() << "addMatch(data)" << endl;
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker*
NepomukQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    debug() << "addFilter()" << endl;
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( matchBegin )
	Q_UNUSED( matchEnd )
	return this;
}

QueryMaker*
NepomukQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    debug() << "excludeFilter()" << endl;
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( matchBegin )
	Q_UNUSED( matchEnd )
	return this;
}

QueryMaker*
NepomukQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    debug() << "addNumberFilter()" << endl;
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( compare )
	return this;
}

QueryMaker*
NepomukQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    debug() << "excludeNumberFilter()" << endl;
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( compare )
	return this;
}

QueryMaker*
NepomukQueryMaker::addReturnValue( qint64 value )
{
    debug() << "addReturnValue()" << endl;
	Q_UNUSED( value )
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    debug() << "addReturnFunction()" << endl;
	Q_UNUSED( function )
	Q_UNUSED( value )
    return this;
}

QueryMaker*
NepomukQueryMaker::orderBy( qint64 value, bool descending )
{
    debug() << "orderBy()" << endl;
	Q_UNUSED( value )
	Q_UNUSED( descending )
    return this;
}

QueryMaker*
NepomukQueryMaker::limitMaxResultSize( int size )
{
    debug() << "limitMaxResultSize()" << endl;
	Q_UNUSED( size )
    return this;
}

/*
QueryMaker*
NepomukQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
	Q_UNUSED( mode )
    return this;
}
*/

QueryMaker*
NepomukQueryMaker::beginAnd()
{
    debug() << "beginAnd()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::beginOr()
{
    debug() << "beginOr()" << endl;
    return this;
}

QueryMaker*
NepomukQueryMaker::endAndOr()
{
    debug() << "endAndOr()" << endl;
    return this;
}

void
NepomukQueryMaker::done( ThreadWeaver::Job *job )
{
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    worker = 0;
    emit queryDone();
}

QString
NepomukQueryMaker::buildQuery() const
{
    QString query;
    
    switch( queryType )
    {
        case ARTIST:
            query  =   QString(
                    "select distinct ?artist where { "
                    "?r <%1> ?artist .}")
                    .arg( Soprano::Vocabulary::Xesam::artist().toString() );
            break;
        case ALBUM:
            
            if ( queryMatch.isEmpty() )
                return query;
            
            query  =   QString(
                    "select distinct ?artist ?album where { "
                    "?r <%1> ?album . "
                    "?r <%2> ?artist . ")
                    .arg( Soprano::Vocabulary::Xesam::album().toString() )
                    .arg( Soprano::Vocabulary::Xesam::artist().toString() );
            query += queryMatch;
            query += '}';
            break;
        case TRACK:
            
            if ( queryMatch.isEmpty() )
                return query;
            
            query  =   QString( "select ?r where { "  );
            query += queryMatch;
            query += '}';
            
        default:
            debug() << "unknown query type" << endl;        
       
    }

    return query;
}

void
NepomukQueryMaker::doQuery(const QString &query)
{
    DEBUG_BLOCK
    if ( query.isEmpty() )
        return;
    
    if (!client->isValid())
    {
        m_collection->lostDBusConnection();
        return;
    }
    
    // some problem with qlocalsocket and threads
    //Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();
    // using dbus (slower) for now
    
    //Soprano::Client::DBusClient* client = new Soprano::Client::DBusClient( "org.kde.NepomukStorage" );

    Soprano::Model* model = (Soprano::Model*)client->createModel( "main" );
    
    switch ( queryType )
    {
        case ARTIST:
        {
            ArtistList al;
            Soprano::QueryResultIterator it
                      = model->executeQuery( query, 
                                             Soprano::Query::QueryLanguageSparql );
            while( it.next() ) 
            {
                Soprano::Node node = it.binding( "artist" ) ;
                al.append( *(new Meta::ArtistPtr(new NepomukArtist(node.toString()))) );
            }
            
            DataList data; 
            foreach( ArtistPtr p, al ) 
            { 
                data << DataPtr::staticCast( p ); 
            } 
            
            emit newResultReady( m_collection->collectionId(), data );
            
            break;
        }
        
        case ALBUM:
        {
            AlbumList al;
            Soprano::QueryResultIterator it
                      = model->executeQuery( query, 
                                             Soprano::Query::QueryLanguageSparql );
            while( it.next() ) 
            {
                QString artist =  it.binding( "artist" ).toString();
                QString album =  it.binding( "album" ).toString();
                al.append( *( new Meta::AlbumPtr( new NepomukAlbum( album, artist ) ) ) );
            }
            
            DataList data; 
            foreach( AlbumPtr p, al ) 
            { 
                data << DataPtr::staticCast( p ); 
            } 
            
            emit newResultReady( m_collection->collectionId(), data );
            
            break;
        }
        
        case TRACK:
        {
            TrackList tl;
            Soprano::QueryResultIterator it
                      = model->executeQuery( query, 
                                             Soprano::Query::QueryLanguageSparql );
            while( it.next() ) 
            {
                Nepomuk::Resource res = Nepomuk::Resource( it.binding( "r" ).uri() );
                KUrl url = res.resourceUri();
                QString title = res.property( Soprano::Vocabulary::Xesam::title() )
                                        .toString();
                QString artist = res.property( Soprano::Vocabulary::Xesam::artist() )
                                        .toString();
                QString album = res.property( Soprano::Vocabulary::Xesam::album() )
                                        .toString();
                QString genre = res.property( Soprano::Vocabulary::Xesam::genre() )
                                        .toString();
                QString composer = res.property( Soprano::Vocabulary::Xesam::composer() )
                                        .toString();
                QString year;
                QString type = res.property( Soprano::Vocabulary::Xesam::fileExtension() )
                                        .toString();
                int trackNumber = res.property( Soprano::Vocabulary::Xesam::trackNumber() )
                                        .toInt();
                int length = res.property( Soprano::Vocabulary::Xesam::mediaDuration() )
                                        .toInt();
                NepomukTrack *np = new Meta::NepomukTrack( url, title, artist, album, genre, 
                                                    year, composer, type, trackNumber, length );
                tl.append( *( new Meta::TrackPtr ( np ) ) );
            }
            
            DataList data; 
            foreach( TrackPtr p, tl ) 
            { 
                data << DataPtr::staticCast( p ); 
            } 
            
            emit newResultReady( m_collection->collectionId(), data );
            
            
            break;
        }
            
        default:
            return;
    }
    
    if ( model ) 
        delete model;

}

#include "NepomukQueryMaker.moc"
