/****************************************************************************************
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

#include "MetaQueryMaker.h"


using namespace Meta;

MetaQueryMaker::MetaQueryMaker( const QList<Amarok::Collection*> &collections )
    : QueryMaker()
    , m_queryDoneCount( 0 )
    , m_queryDoneCountMutex()
{
    foreach( Amarok::Collection *c, collections )
    {
        QueryMaker *b = c->queryMaker();
        builders.append( b );
        connect( b, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ) );
        //relay signals directly
        connect( b, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SIGNAL( newResultReady( QString, Meta::TrackList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), this, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), this, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::GenreList ) ), this, SIGNAL( newResultReady( QString, Meta::GenreList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), this, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::YearList ) ), this, SIGNAL( newResultReady( QString, Meta::YearList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, QStringList ) ), this, SIGNAL( newResultReady( QString, QStringList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::DataList ) ), this, SIGNAL( newResultReady( QString, Meta::DataList ) ), Qt::DirectConnection );
    }
}

MetaQueryMaker::MetaQueryMaker( const QList<QueryMaker*> &queryMakers )
    : QueryMaker()
    , builders( queryMakers )
    , m_queryDoneCount( 0 )
    , m_queryDoneCountMutex()
{
    foreach( QueryMaker *b, builders )
    {
        connect( b, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ) );
        //relay signals directly
        connect( b, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SIGNAL( newResultReady( QString, Meta::TrackList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), this, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), this, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::GenreList ) ), this, SIGNAL( newResultReady( QString, Meta::GenreList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), this, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::YearList ) ), this, SIGNAL( newResultReady( QString, Meta::YearList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, QStringList ) ), this, SIGNAL( newResultReady( QString, QStringList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::DataList ) ), this, SIGNAL( newResultReady( QString, Meta::DataList ) ), Qt::DirectConnection );
    }
}

MetaQueryMaker::~MetaQueryMaker()
{
    foreach( QueryMaker *b, builders )
        delete b;
}

QueryMaker*
MetaQueryMaker::reset()
{
    m_queryDoneCount = 0;
    foreach( QueryMaker *b, builders )
        b->reset();
    return this;
}

void
MetaQueryMaker::run()
{
    foreach( QueryMaker *b, builders )
        b->run();
}

void
MetaQueryMaker::abortQuery()
{
    foreach( QueryMaker *b, builders )
        b->abortQuery();
}

int
MetaQueryMaker::resultCount() const
{
    int count = 0;
    foreach( QueryMaker *b, builders )
    {
        count += b->resultCount();
    }
    return count;
}

QueryMaker*
MetaQueryMaker::setQueryType( QueryType type )
{
    foreach( QueryMaker *qm, builders )
        qm->setQueryType( type );
    return this;
}

QueryMaker*
MetaQueryMaker::addReturnValue( qint64 value )
{
    foreach( QueryMaker *b, builders )
        b->addReturnValue( value );
    return this;
}

QueryMaker*
MetaQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    foreach( QueryMaker *qm, builders )
        qm->addReturnFunction( function, value );
    return this;
}

QueryMaker*
MetaQueryMaker::orderBy( qint64 value, bool descending )
{
    foreach( QueryMaker *b, builders )
        b->orderBy( value, descending );
    return this;
}

QueryMaker*
MetaQueryMaker::orderByRandom()
{
    foreach( QueryMaker *b, builders )
        b->orderByRandom();
    return this;
}

QueryMaker*
MetaQueryMaker::includeCollection( const QString &collectionId )
{
    foreach( QueryMaker *b, builders )
        b->includeCollection( collectionId );
    return this;
}

QueryMaker*
MetaQueryMaker::excludeCollection( const QString &collectionId )
{
    foreach( QueryMaker *b, builders )
        b->excludeCollection( collectionId );
    return this;
}

QueryMaker*
MetaQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    foreach( QueryMaker *b, builders )
        b->addFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
MetaQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    foreach( QueryMaker *b, builders )
        b->excludeFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
MetaQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    foreach( QueryMaker *b, builders )
        b->addNumberFilter( value, filter, compare);
    return this;
}

QueryMaker*
MetaQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    foreach( QueryMaker *b, builders )
        b->excludeNumberFilter( value, filter, compare );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const TrackPtr &track )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( track );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const ArtistPtr &artist )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( artist );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const AlbumPtr &album )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( album );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const GenrePtr &genre )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( genre );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const ComposerPtr &composer )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( composer );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const YearPtr &year )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( year );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const DataPtr &data )
{
    DataPtr tmp = const_cast<DataPtr&>( data );
    foreach( QueryMaker *b, builders )
        tmp->addMatchTo( b );
    return this;
}

QueryMaker*
MetaQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    foreach( QueryMaker *b, builders )
        b->setReturnResultAsDataPtrs( resultAsDataPtrs );
    return this;
}

QueryMaker*
MetaQueryMaker::limitMaxResultSize( int size )
{
    foreach( QueryMaker *b, builders )
        b->limitMaxResultSize( size );
    return this;
}

QueryMaker*
MetaQueryMaker::beginAnd()
{
    foreach( QueryMaker *b, builders )
        b->beginAnd();
    return this;
}

QueryMaker*
MetaQueryMaker::beginOr()
{
    foreach( QueryMaker *b, builders )
        b->beginOr();
    return this;
}

QueryMaker*
MetaQueryMaker::endAndOr()
{
    foreach( QueryMaker *b, builders )
        b->endAndOr();
    return this;
}

QueryMaker*
MetaQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    foreach( QueryMaker *qm, builders )
        qm->setAlbumQueryMode( mode );
    return this;
}

void
MetaQueryMaker::slotQueryDone()
{
    m_queryDoneCountMutex.lock();
    m_queryDoneCount++;
    if ( m_queryDoneCount == builders.size() )
    {
        //make sure we don't give control to code outside this class while holding the lock
        m_queryDoneCountMutex.unlock();
        emit queryDone();
    }
    else
        m_queryDoneCountMutex.unlock();
}

#include "MetaQueryMaker.moc"

