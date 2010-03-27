/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukQueryMaker.h"

#include "NepomukAlbum.h"
#include "NepomukCollection.h"
#include "NepomukRegistry.h"

#include "core/support/Debug.h"

#include <QString>
#include <QStringList>

#include <KUrl>
#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
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



NepomukQueryMaker::NepomukQueryMaker(NepomukCollection *collection
            , Soprano::Model* model)
    : QueryMaker() 
    , m_collection(collection)
    , m_model( model )
{
    m_worker = 0;
    reset();
}

NepomukQueryMaker::~NepomukQueryMaker()
{
    
}

QueryMaker*
NepomukQueryMaker::reset()
{
    m_used=false;
    m_data.clear();
    m_queryType = None;
    m_queryMatch.clear();
    m_queryFilter.clear();
    if( m_worker && m_worker->isFinished() )
        delete m_worker;   //TODO error handling
    this->m_resultAsDataPtrs = false;
    m_queryOrderBy.clear();
    m_queryLimit = 0;
    m_blocking = false;
    m_andStack.clear();
    m_andStack.push( true );   //and is default
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
    if( m_queryType == None || m_used )
    {
        debug() << "nepomuk querymaker used without reset or initialization" << endl;
        return; //better error handling?
    } 
    
    if( m_worker && !m_worker->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait for job to complete
    }
    else if ( !m_blocking )
    {
        m_worker = new NepomukWorkerThread(this);
        connect( m_worker, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( done( ThreadWeaver::Job* ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( m_worker );
    }
    else
    {
        QString query = buildQuery();
        doQuery( query );

    }
    m_used = true;
}

QueryMaker*
NepomukQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    debug() << "setReturnResultAsDataPtrs()" << resultAsDataPtrs << endl;

    // we need the unchanged resulttype in the blocking result methods so prevent
    // reseting result type without reseting the QM
    if ( m_used )
        return this;
    
    this->m_resultAsDataPtrs = resultAsDataPtrs;
    return this;
}

QueryMaker*
NepomukQueryMaker::setQueryType( QueryType type )
{
    // we need the unchanged m_queryType in the blocking result methods so prevent
    // reseting m_queryType without reseting the QM
    if ( m_used )
        return this;
    
    m_queryType = type;
    switch( type )
    {
    case QueryMaker::Track:
        debug() << "startTrackQuery()" << endl;
        m_queryType  = Track;
    
        // FIXME: This breaks controllable sorting for tracks
        // but works around collections views assumptions about
        // the order 
        // should be fixed there and then removed here
        orderBy(Meta::valAlbum);
        orderBy(Meta::valTrackNr);
        return this;
    case QueryMaker::Artist:
        m_queryType = Artist;
        debug() << "startArtistQuery()" << endl;
        return this;
    case QueryMaker::Album:
        m_queryType = Album;
        debug() << "starAlbumQuery()" << endl;
        return this;
    case QueryMaker::Genre:
        debug() << "startGenreQuery()" << endl;
        return this;
    case QueryMaker::Composer:
        debug() << "startComposerQuery()" << endl;
        return this;
    case QueryMaker::Year:
        debug() << "startYearQuery()" << endl;
        return this;
    case QueryMaker::Custom:
        debug() << "startCustomQuery()" << endl;
        return this;
     default:
        return this;       
    }
}

QueryMaker*
NepomukQueryMaker::includeCollection( const QString &collectionId )
{
    // TODO:  Find out what it is for. Seems to do nothing in SqlCollection
    debug() << "includeCollection()" << endl;
	Q_UNUSED( collectionId )
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeCollection( const QString &collectionId )
{
    // TODO:  Find out what it is for. Seems to do nothing in SqlCollection
    debug() << "excludeCollection()" << endl;
	Q_UNUSED( collectionId )
	return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const TrackPtr &track )
{
    debug() << "addMatch(Track)" << endl;
    m_queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( m_collection->getUrlForValue( valUrl ) )
            .arg( track->uidUrl() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const ArtistPtr &artist )
{
    debug() << "addMatch(artist)" << endl;
    m_queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( m_collection->getUrlForValue( valArtist) )
            .arg( artist->name() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );

    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const AlbumPtr &album )
{
    debug() << "addMatch(album)" << endl;

    m_queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( m_collection->getUrlForValue( valAlbum ) )
            .arg( album->name() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const GenrePtr &genre )
{
    debug() << "addMatch(genre)" << endl;
    m_queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( m_collection->getUrlForValue( valGenre ) )
            .arg( genre->name() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const ComposerPtr &composer )
{
    debug() << "addMatch(composer)" << endl;
    
    m_queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( m_collection->getUrlForValue( valComposer ) )
            .arg( composer->name() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const YearPtr &year )
{
    debug() << "addMatch(year)" << endl;

    m_queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( m_collection->getUrlForValue( valYear ) )
            .arg( year->name() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
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
NepomukQueryMaker::addMatch( const KUrl &url )
{
    m_queryMatch +=  QString(
            " ?r <%1> \"%2\"^^<%3> . ")
            .arg( m_collection->getUrlForValue( valUrl ) )
            .arg( url.pathOrUrl() )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatchId( const QString &uid )
{
    m_queryMatch +=  QString(
                           " ?r <%1> \"%2\"^^<%3> . ")
            .arg( "http://amarok.kde.org/metadata/1.0/track#uid" )
            .arg( uid )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
    return this;
}

QueryMaker*
NepomukQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    debug() << m_queryType << " addFilter()" << endl;
    debug() << m_queryType << "filter against: " << m_collection->getNameForValue( value ) << endl;
    debug() <<  m_queryType <<  "filter: " << filter << endl;
    debug() <<  m_queryType << "matchbegin, match end " << matchBegin << matchEnd << endl;

    QString like = likeCondition( filter, matchBegin, matchEnd );
    m_queryFilter += QString( " %1 REGEX ( STR( ?%2 ) , \"%3\" , \"i\" ) " ).arg( andOr(), m_collection->getNameForValue( value ), like );
    addEmptyMatch( value );
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
    if ( m_queryOrderBy.isEmpty() )
        m_queryOrderBy = " ORDER BY ";
    m_queryOrderBy += descending ? "DESC(?" : "ASC(?" ;
    m_queryOrderBy += m_collection->getNameForValue( value ) + ") ";
    return this; 
}

QueryMaker*
NepomukQueryMaker::orderByRandom()
{
    // lets see if they are random enough
    m_queryOrderBy.clear();
    return this;
}

QueryMaker*
NepomukQueryMaker::limitMaxResultSize( int size )
{
    debug() << "limitMaxResultSize()" << endl;
	m_queryLimit = size;
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
    debug() <<  m_queryType <<  "beginAnd()" << endl;
    m_queryFilter += andOr();
    m_queryFilter += " ( 1 ";
    m_andStack.push( true );
    return this;
}

QueryMaker*
NepomukQueryMaker::beginOr()
{
    debug() <<  m_queryType <<  "beginOr()" << endl;
    m_queryFilter += andOr();
    m_queryFilter += " ( 0 ";
    m_andStack.push( false );
    return this;
}

QueryMaker*
NepomukQueryMaker::endAndOr()
{
    debug() <<  m_queryType << "endAndOr()" << endl;
    m_queryFilter += ')';
    m_andStack.pop();
    return this;
}

void
NepomukQueryMaker::done( ThreadWeaver::Job *job )
{
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    m_worker = 0;
    emit queryDone();
}

QString
NepomukQueryMaker::likeCondition( const QString &text, bool matchBegin, bool matchEnd ) const
{
    QString ret = text;
    if ( matchBegin )
        ret = '^' + ret;
    if ( matchEnd )
        ret +=  '$';
    
    return ret;
}

QString
NepomukQueryMaker::andOr() const
{
    return m_andStack.top() ? " && " : " || ";
}

void
NepomukQueryMaker::addEmptyMatch( const qint64 value, bool optional )
{
    // TODO: only add values which are not already part of the query
    m_queryMatch += QString( " ?r <%1> ?%2 . " ).arg( m_collection->getUrlForValue( value ), m_collection->getNameForValue( value ) );
    if ( optional )
        m_queryMatch += QString( " OPTIONAL {%1} . " ).arg( m_queryMatch );
}

void
NepomukQueryMaker::blocking( bool enabled )
{
    m_blocking = enabled;
}

QStringList
 NepomukQueryMaker::collectionIds() const
{
    QStringList list;
    list << m_collection->collectionId();
    return list;
}

Meta::DataList
NepomukQueryMaker::data( const QString &id ) const
{
    if ( m_blocking && m_used && m_resultAsDataPtrs && m_collection->collectionId() == id )
        return m_data;
    else
        return Meta::DataList();
}

Meta::TrackList
NepomukQueryMaker::tracks( const QString &id ) const
{
    if ( m_blocking && m_used && m_queryType == QueryMaker::Track && m_collection->collectionId() == id  )
    {
        Meta::TrackList list;
        foreach( DataPtr p, m_data )
        { 
            list << Meta::TrackPtr::staticCast( p ); \
        } 
        return list;
    }
    else
        return Meta::TrackList();
}

Meta::AlbumList
NepomukQueryMaker::albums( const QString &id ) const
{
    if ( m_blocking && m_used && m_queryType == QueryMaker::Album && m_collection->collectionId() == id  )
    {
        Meta::AlbumList list;
        foreach( DataPtr p, m_data )
        {
            list << Meta::AlbumPtr::staticCast( p ); \
        }
        return list;
    }
    else
        return Meta::AlbumList();
}

Meta::ArtistList
NepomukQueryMaker::artists( const QString &id ) const
{
    if ( m_blocking && m_used && m_queryType == QueryMaker::Artist && m_collection->collectionId() == id  )
    {
        Meta::ArtistList list;
        foreach( DataPtr p, m_data )
        {
            list << Meta::ArtistPtr::staticCast( p ); \
        }
        return list;
    }
    else
        return Meta::ArtistList();
}

Meta::GenreList
NepomukQueryMaker::genres( const QString &id ) const
{
    Q_UNUSED( id )
    // not implemented yet
    return Meta::GenreList();
}

Meta::ComposerList
NepomukQueryMaker::composers( const QString &id ) const
{
    Q_UNUSED( id )
    // not implemented yet
    return Meta::ComposerList();
}

Meta::YearList
NepomukQueryMaker::years( const QString &id ) const
{
    Q_UNUSED( id )
    // not implemented yet
    return Meta::YearList();
}

QStringList
NepomukQueryMaker::customData( const QString &id ) const
{
    Q_UNUSED( id )
    // not implemented yet
    return QStringList();
}

QString
NepomukQueryMaker::buildQuery()
{
    QString query;
    
    switch( m_queryType )
    {
        case Artist:
            query  =  "select distinct ?artist where { ";
            addEmptyMatch( valArtist );
            query += m_queryMatch;

            break;
        case Album:
            if ( m_queryMatch.isEmpty() && m_queryFilter.isEmpty() )
                            return query;
            query  =  "select distinct ?artist ?album where { ";
            addEmptyMatch( valAlbum );
            addEmptyMatch( valArtist );
            query += m_queryMatch;

            break;
        case Track:
        {
            query  = "SELECT * WHERE { ";
            addEmptyMatch( valUrl );
            
            // if there is no match we want all tracks, match against all music
            if ( m_queryMatch.isEmpty() )
                query += QString( "?r a <%1> . ")
                        .arg( Soprano::Vocabulary::Xesam::Music().toString() );
            else
                query += m_queryMatch;
            
            const QStringList list = m_collection->getAllNamesAndUrls();
            QStringList::const_iterator it = list.constBegin();
            while ( it != list.constEnd() )
            {
                query += QString("OPTIONAL { ?r <%1> ?%2 } . ")
                        .arg( *(it++) )
                        .arg( *(it++) );
            }
            break;
        }
        default:
            debug() << "unknown query type" << endl;        
    }
    
    if ( !m_queryFilter.isEmpty() )
        query += QString( " FILTER( 1 %1 ) " ).arg(  m_queryFilter );
    query += " } ";
    query += m_queryOrderBy;
    if (m_queryLimit != 0 )
        query += QString( " LIMIT %1 ").arg( m_queryLimit );
    return query;
}

// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.

#define emitOrStoreProperResult( PointerType, list ) { \
            if ( m_resultAsDataPtrs || m_blocking ) { \
                foreach( PointerType p, list ) { \
                    m_data << DataPtr::staticCast( p ); \
                } \
                if ( !m_blocking ) \
                    emit newResultReady( m_collection->collectionId(), m_data ); \
            } \
            else { \
                emit newResultReady( m_collection->collectionId(), list ); \
            } \
        }


void
NepomukQueryMaker::doQuery(const QString &query)
{
    DEBUG_BLOCK
    if ( query.isEmpty() )
        return;
    switch ( m_queryType )
    {
        case Artist:
        {
            ArtistList al;
            Soprano::QueryResultIterator it
                      = m_model->executeQuery( query, 
                                             Soprano::Query::QueryLanguageSparql );
            while( it.next() ) 
            {
                Soprano::Node node = it.binding( "artist" ) ;
                Meta::ArtistPtr ap = m_collection->registry()->artistForArtistName( node.toString() );
                al.append( ap );
            }
            emitOrStoreProperResult ( ArtistPtr, al );
            
            break;
        }
        case Album:
        {
            AlbumList al;
            Soprano::QueryResultIterator it
                      = m_model->executeQuery( query, 
                                             Soprano::Query::QueryLanguageSparql );
            while( it.next() ) 
            {
                QString artist =  it.binding( "artist" ).toString();
                QString album =  it.binding( "album" ).toString();
                Meta::AlbumPtr ap = m_collection->registry()->albumForArtistAlbum( artist, album );
                al.append( ap );
            }         
            emitOrStoreProperResult ( AlbumPtr, al );
            
            break;
        }
        
        case Track:
        {
            TrackList tl;
            Soprano::QueryResultIterator it
                      = m_model->executeQuery( query, 
                                             Soprano::Query::QueryLanguageSparql );
            while( it.next() ) 
            {
                Soprano::BindingSet bindingSet = it.currentBindings();
                
                Meta::TrackPtr np = m_collection->registry()->trackForBindingSet( bindingSet );
                tl.append( np );
            } emitOrStoreProperResult ( TrackPtr, tl );

            break;
        }
            
        default:
            return;
    }
}
#include "NepomukQueryMaker.moc"

