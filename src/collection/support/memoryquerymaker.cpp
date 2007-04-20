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

MemoryQueryMaker::MemoryQueryMaker( MemoryCollection *mc )
    : QueryMaker()
    , m_memCollection( mc )
    ,d( new Private )
{
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
}

void
MemoryQueryMaker::abortQuery()
{
}

void
MemoryQueryMaker::runQuery()
{
    m_memCollection->acquireReadLock();
    m_memCollection->releaseLock();
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

