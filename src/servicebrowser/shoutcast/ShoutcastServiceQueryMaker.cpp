/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2007 Adam Pigg <adam@piggz.co.uk>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "ShoutcastServiceQueryMaker.h"

#include "amarok.h"
#include "debug.h"
#include "servicemetabase.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

 #include <QDomDocument>

using namespace Meta;

class QueryJob : public ThreadWeaver::Job
{
    public:
        QueryJob( ShoutcastServiceQueryMaker *qm )
            : ThreadWeaver::Job()
            , m_queryMaker( qm )
        {
            //nothing to do
        }

    public slots:

        void taskDone()
        {
            setFinished( true );
        }

    protected:
        void run()
        {
            DEBUG_BLOCK
            connect( m_queryMaker, SIGNAL( dynamicQueryComplete() ), this, SLOT( taskDone() ) );
            m_queryMaker->runQuery();
        }

    private:
        ShoutcastServiceQueryMaker *m_queryMaker;
};



struct ShoutcastServiceQueryMaker::Private {
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    QueryType type;
    QueryJob *job;
    int maxsize;
    bool returnDataPtrs;
};


ShoutcastServiceQueryMaker::ShoutcastServiceQueryMaker( ShoutcastServiceCollection * collection )
 : QueryMaker()
 , d( new Private )
{
    m_collection = collection;
    d->job = 0;
    reset();
}


ShoutcastServiceQueryMaker::~ShoutcastServiceQueryMaker()
{
}

QueryMaker * ShoutcastServiceQueryMaker::reset()
{
    d->type = Private::NONE;
    delete d->job;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    m_genreMatch = QString();

    return this;
}

QueryMaker*
ShoutcastServiceQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

void ShoutcastServiceQueryMaker::run()
{

    if ( d->type == Private::NONE )
        //TODO error handling
        return;
    else if( d->job && !d->job->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait for job to complete
    }
    else
    {
       /* d->job = new QueryJob( this );
        connect( d->job, SIGNAL( done( ThreadWeaver::Job * ) ), SLOT( done( ThreadWeaver::Job * ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( d->job );*/

        fetchGenres();
    }
}

void ShoutcastServiceQueryMaker::runQuery()
{
    DEBUG_BLOCK
    m_collection->acquireReadLock();
    //naive implementation, fix this
    //note: we are not handling filtering yet
  
    //this is where the fun stuff happens
    if (  d->type == Private::GENRE )
        fetchGenres();

    m_collection->releaseLock();
}


void ShoutcastServiceQueryMaker::abortQuery()
{
}

QueryMaker * ShoutcastServiceQueryMaker::startTrackQuery()
{
    DEBUG_BLOCK
    d->type = Private::TRACK;
}

QueryMaker * ShoutcastServiceQueryMaker::startGenreQuery()
{
    DEBUG_BLOCK
    d->type = Private::GENRE;
}

QueryMaker * ShoutcastServiceQueryMaker::addMatch(const Meta::GenrePtr & genre)
{
    m_genreMatch = genre->name();
}



// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.
// (copied from sqlquerybuilder.cpp with a few minor tweaks)

#define emitProperResult( PointerType, list ) { \
            if ( d->returnDataPtrs ) { \
                DataList data; \
                foreach( PointerType p, list ) { \
                    data << DataPtr::staticCast( p ); \
                } \
                emit newResultReady( m_collection->collectionId(), data ); \
            } \
            else { \
                emit newResultReady( m_collection->collectionId(), list ); \
            } \
        }


void ShoutcastServiceQueryMaker::handleResult() {
    DEBUG_BLOCK
    switch( d->type )
    {
        case Private::GENRE :
        {
            GenreList genres = m_collection->genreMap().values();
            if ( d->maxsize >= 0 && genres.count() > d->maxsize )
                genres = genres.mid( 0, d->maxsize );
            emitProperResult( GenrePtr, genres );
            break;
        }
    }
}


void ShoutcastServiceQueryMaker::fetchGenres()
{
    DEBUG_BLOCK
    m_storedTransferJob =  KIO::storedGet(  KUrl( "http://www.shoutcast.com/sbin/newxml.phtml" ), false, true );
    connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
        , this, SLOT( genreDownloadComplete(KJob *) ) );

}

void ShoutcastServiceQueryMaker::genreDownloadComplete(KJob * job)
{

    DEBUG_BLOCK

    QDomDocument doc( "genres" );

    doc.setContent( m_storedTransferJob->data() );

    debug() << "So far so good... Got this data: " << m_storedTransferJob->data();


    // We use this list to filter out some obscure genres
    QStringList bannedGenres;
    bannedGenres << "alles" << "any" << "anything" << "autopilot" << "backup" << "bandas" << "beer";
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
    bannedGenres << "max" << "informaci" << "halk" << "dobra" << "welcome" << "genre";


    GenreMap genreMap;

    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        const QString name = e.attribute( "name" );
        if( !name.isNull() && !bannedGenres.contains( name.toLower() ) && !genreMap.contains( name ) )
        {

            debug() << "add genre: " << name;

            ServiceGenre * genre = new ServiceGenre( name );
            GenrePtr genrePtr( genre );
            m_collection->addGenre( name,  genrePtr );

        }
        n = n.nextSibling();
    }

    handleResult();
    emit queryDone();
    //emit( dynamicQueryComplete() );


}



void
ShoutcastServiceQueryMaker::done( ThreadWeaver::Job *job )
{
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    d->job = 0;
    emit queryDone();
}

#include "ShoutcastServiceQueryMaker.moc"






