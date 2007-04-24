/* 
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

#include "memoryquerymaker.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QSet>

Matcher::Matcher()
    : m_next( 0 )
{
}

Matcher::~Matcher()
{
    delete m_next;
}

bool
Matcher::isLast() const
{
    return !m_next;
}

Matcher*
Matcher::next() const
{
    return m_next;
}

void
Matcher::setNext( Matcher *next )
{
    if ( !m_next )
        delete m_next;
    m_next = next;
}

//TODO check if it's possible to use templates here

class ArtistMatcher : public Matcher
{
    public:
     ArtistMatcher( ArtistPtr artist )
        : Matcher()
        , m_artist( artist )
    {}

    virtual TrackList match( MemoryCollection *memColl )
    {
        ArtistMap artistMap = memColl->artistMap();
        if ( artistMap.contains( m_artist->name() ) )
        {
            ArtistPtr artist = artistMap.value( m_artist->name() );
            TrackList matchingTracks = artist->tracks();
            if ( isLast() )
                return matchingTracks;
            else
                return next()->match( matchingTracks );
        }
        else
            return TrackList();
    }

    virtual TrackList match( const TrackList &tracks )
    {
        TrackList matchingTracks;
        QString name = m_artist->name();
        foreach( TrackPtr track, tracks )
            if ( track->artist()->name() == name )
                matchingTracks.append( track );
        if ( isLast() || matchingTracks.count() == 0)
            return matchingTracks;
        else
            return next()->match( matchingTracks );
    }

    private:
        ArtistPtr m_artist;
};

class AlbumMatcher : public Matcher
{
    public:
        AlbumMatcher( AlbumPtr album )
        : Matcher()
        , m_album( album )
    {}

    virtual TrackList match( MemoryCollection *memColl )
    {
        AlbumMap albumMap = memColl->albumMap();
        if ( albumMap.contains( m_album->name() ) )
        {
            AlbumPtr album = albumMap.value( m_album->name() );
            TrackList matchingTracks = album->tracks();
            if ( isLast() )
                return matchingTracks;
            else
                return next()->match( matchingTracks );
        }
        else
            return TrackList();
    }

    virtual TrackList match( const TrackList &tracks )
    {
        TrackList matchingTracks;
        QString name = m_album->name();
        foreach( TrackPtr track, tracks )
            if ( track->album()->name() == name )
                matchingTracks.append( track );
        if ( isLast() || matchingTracks.count() == 0)
            return matchingTracks;
        else
            return next()->match( matchingTracks );
    }

    private:
        AlbumPtr m_album;
};

//QueryJob

class QueryJob : public ThreadWeaver::Job
{
    public:
        QueryJob( MemoryQueryMaker *qm )
            : ThreadWeaver::Job()
            , m_queryMaker( qm )
        {
            //nothing to do
        }

    protected:
        void run()
        {
            m_queryMaker->runQuery();
            setFinished( true );
        }

    private:
        MemoryQueryMaker *m_queryMaker;
};

struct MemoryQueryMaker::Private {
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    QueryType type;
    bool returnDataPtrs;
    Matcher* matcher;
};

MemoryQueryMaker::MemoryQueryMaker( MemoryCollection *mc, const QString &collectionId )
    : QueryMaker()
    , m_memCollection( mc )
    , m_collectionId( collectionId )
    ,d( new Private )
{
    d->matcher = 0;
    reset();
}

MemoryQueryMaker::~MemoryQueryMaker()
{
    delete d;
}

QueryMaker*
MemoryQueryMaker::reset()
{
    d->type = Private::NONE;
    d->returnDataPtrs = false;
    delete d->matcher;
    return this;
}

void
MemoryQueryMaker::run()
{
    if ( d->type == Private::NONE )
        //TODO error handling
        return;
    else
    {
    }
}

void
MemoryQueryMaker::abortQuery()
{
}

void
MemoryQueryMaker::runQuery()
{
    m_memCollection->acquireReadLock();
    //naive implementation, fix this
    //note: we are not handling filtering yet
    if ( d->matcher )
    {
        TrackList result = d->matcher->match( m_memCollection );
        handleResult( result );
    }
    else
        handleResult();
    m_memCollection->releaseLock();
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
                emit newResultReady( m_collectionId, data ); \
            } \
            else { \
                emit newResultReady( m_collectionId, list ); \
            } \
        }

void
MemoryQueryMaker::handleResult()
{
    //this gets called when we want to return all values for the given query type
    switch( d->type )
    {
        case Private::TRACK :
            emitProperResult( TrackPtr, m_memCollection->trackMap().values() );
            break;
        case Private::ALBUM :
            emitProperResult( AlbumPtr, m_memCollection->albumMap().values() );
            break;
        case Private::ARTIST :
            emitProperResult( ArtistPtr, m_memCollection->artistMap().values() );
            break;
        case Private::COMPOSER :
            emitProperResult( ComposerPtr, m_memCollection->composerMap().values() );
            break;
        case Private::GENRE :
            emitProperResult( GenrePtr, m_memCollection->genreMap().values() );
            break;
        case Private::YEAR :
            emitProperResult( YearPtr, m_memCollection->yearMap().values() );
            break;
        case Private::CUSTOM :
            //TODO stub, fix this
            break;
        case Private::NONE :
            //nothing to do
            break;
    }
    emit queryDone();
}

void
MemoryQueryMaker::handleResult( const TrackList &tracks )
{
    switch( d->type )
    {
        case Private::TRACK :
            emitProperResult( TrackPtr, tracks );
            break;
        case Private::ALBUM :
        {
            QSet<AlbumPtr> albumSet;
            foreach( TrackPtr track, tracks )
                albumSet.insert( track->album() );
            emitProperResult( AlbumPtr, albumSet.toList() );
            break;
        }
        case Private::ARTIST :
        {
            QSet<ArtistPtr> artistSet;
            foreach( TrackPtr track, tracks )
                artistSet.insert( track->artist() );
            emitProperResult( ArtistPtr, artistSet.toList() );
            break;
        }
        case Private::GENRE :
        {
            QSet<GenrePtr> genreSet;
            foreach( TrackPtr track, tracks )
                genreSet.insert( track->genre() );
            emitProperResult( GenrePtr, genreSet.toList() );
            break;
        }
        case Private::COMPOSER :
        {
            QSet<ComposerPtr> composerSet;
            foreach( TrackPtr track, tracks )
                composerSet.insert( track->composer() );
            emitProperResult( ComposerPtr, composerSet.toList() );
            break;
        }
        case Private::YEAR :
        {
            QSet<YearPtr> yearSet;
            foreach( TrackPtr track, tracks )
                yearSet.insert( track->year() );
            emitProperResult( YearPtr, yearSet.toList() );
            break;
        }
        case Private::CUSTOM :
            //hmm, not sure if this makes sense
            break;
        case Private::NONE:
            //should never happen, but handle error anyway
            break;
    }
    emit queryDone();
}

QueryMaker*
MemoryQueryMaker::startTrackQuery()
{
    if ( d->type == Private::NONE )
        d->type = Private::TRACK;
    return this;
}

QueryMaker*
MemoryQueryMaker::startArtistQuery()
{
    if ( d->type == Private::NONE )
        d->type = Private::ARTIST;
    return this;
}

QueryMaker*
MemoryQueryMaker::startAlbumQuery()
{
    if ( d->type == Private::NONE )
        d->type = Private::ALBUM;
    return this;
}

QueryMaker*
MemoryQueryMaker::startComposerQuery()
{
    if ( d->type == Private::NONE )
        d->type = Private::COMPOSER;
    return this;
}

QueryMaker*
MemoryQueryMaker::startGenreQuery()
{
    if ( d->type == Private::NONE )
        d->type = Private::GENRE;
    return this;
}

QueryMaker*
MemoryQueryMaker::startYearQuery()
{
    if ( d->type == Private::NONE )
        d->type = Private::YEAR;
    return this;
}

QueryMaker*
MemoryQueryMaker::startCustomQuery()
{
    if ( d->type == Private::CUSTOM )
        d->type = Private::CUSTOM;
    return this;
}

QueryMaker*
MemoryQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

QueryMaker*
MemoryQueryMaker::addReturnValue( qint64 value )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::orderBy( qint64 value, bool descending )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::includeCollection( const QString &collectionId )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::excludeCollection( const QString &collectionId )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const TrackPtr &track )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const ArtistPtr &artist )
{
    Matcher *artistMatcher = new ArtistMatcher( artist );
    if ( d->matcher == 0 )
        d->matcher = artistMatcher;
    else
    {
        Matcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( artistMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const AlbumPtr &album )
{
    Matcher *albumMatcher = new AlbumMatcher( album );
    if ( d->matcher == 0 )
        d->matcher = albumMatcher;
    else
    {
        Matcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( albumMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const GenrePtr &genre )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const ComposerPtr &composer )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const YearPtr &year )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const DataPtr &data )
{
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker*
MemoryQueryMaker::addFilter( qint64 value, const QString &filter )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::excludeFilter( qint64 value, const QString &filter )
{
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::limitMaxResultSize( int size )
{
    //TODO stub
    return this;
}

