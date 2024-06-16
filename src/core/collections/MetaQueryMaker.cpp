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

#include "core/collections/MetaQueryMaker.h"
#include "core/meta/Meta.h"

using namespace Collections;

MetaQueryMaker::MetaQueryMaker( const QList<Collections::Collection*> &collections )
    : QueryMaker()
    , m_queryDoneCount( 0 )
    , m_queryDoneCountMutex()
{
    for( Collections::Collection *c : collections )
    {
        QueryMaker *b = c->queryMaker();
        builders.append( b );
        connect( b, &QueryMaker::queryDone, this, &MetaQueryMaker::slotQueryDone );
        //relay signals directly
        // actually this is wrong. We would need to combine the results
        // to prevent duplicate album name results.
        // On the other hand we need duplicate AlbumPtr results.
        // Summary: be careful when using this class. (Ralf)
        connect( b, &QueryMaker::newTracksReady, this, &MetaQueryMaker::newTracksReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newArtistsReady, this, &MetaQueryMaker::newArtistsReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newAlbumsReady, this, &MetaQueryMaker::newAlbumsReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newGenresReady, this, &MetaQueryMaker::newGenresReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newComposersReady, this, &MetaQueryMaker::newComposersReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newYearsReady, this, &MetaQueryMaker::newYearsReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newResultReady, this, &MetaQueryMaker::newResultReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newLabelsReady, this, &MetaQueryMaker::newLabelsReady, Qt::DirectConnection );
    }
}

MetaQueryMaker::MetaQueryMaker( const QList<QueryMaker*> &queryMakers )
    : QueryMaker()
    , builders( queryMakers )
    , m_queryDoneCount( 0 )
    , m_queryDoneCountMutex()
{
    for( QueryMaker *b : builders )
    {
        connect( b, &QueryMaker::queryDone, this, &MetaQueryMaker::slotQueryDone );
        //relay signals directly
        connect( b, &QueryMaker::newTracksReady, this, &MetaQueryMaker::newTracksReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newArtistsReady, this, &MetaQueryMaker::newArtistsReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newAlbumsReady, this, &MetaQueryMaker::newAlbumsReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newGenresReady, this, &MetaQueryMaker::newGenresReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newComposersReady, this, &MetaQueryMaker::newComposersReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newYearsReady, this, &MetaQueryMaker::newYearsReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newResultReady, this, &MetaQueryMaker::newResultReady, Qt::DirectConnection );
        connect( b, &QueryMaker::newLabelsReady, this, &MetaQueryMaker::newLabelsReady, Qt::DirectConnection );
    }
}

MetaQueryMaker::~MetaQueryMaker()
{
    qDeleteAll( builders );
}

void
MetaQueryMaker::run()
{
    for( QueryMaker *b : builders )
        b->run();
}

void
MetaQueryMaker::abortQuery()
{
    for( QueryMaker *b : builders )
        b->abortQuery();
}

QueryMaker*
MetaQueryMaker::setQueryType( QueryType type )
{
    for( QueryMaker *qm : builders )
        qm->setQueryType( type );
    return this;
}

QueryMaker*
MetaQueryMaker::addReturnValue( qint64 value )
{
    for( QueryMaker *b : builders )
        b->addReturnValue( value );
    return this;
}

QueryMaker*
MetaQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    for( QueryMaker *qm : builders )
        qm->addReturnFunction( function, value );
    return this;
}

/* Ok. That doesn't work. First connecting the signals directly and then
   doing "orderBy" directly */
QueryMaker*
MetaQueryMaker::orderBy( qint64 value, bool descending )
{
    for( QueryMaker *b : builders )
        b->orderBy( value, descending );
    return this;
}

QueryMaker*
MetaQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    for( QueryMaker *b : builders )
        b->addFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
MetaQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    for( QueryMaker *b : builders )
        b->excludeFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
MetaQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    for( QueryMaker *b : builders )
        b->addNumberFilter( value, filter, compare);
    return this;
}

QueryMaker*
MetaQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    for( QueryMaker *b : builders )
        b->excludeNumberFilter( value, filter, compare );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    for( QueryMaker *b : builders )
        b->addMatch( track );
    return this;
}


QueryMaker*
MetaQueryMaker::addMatch( const Meta::ArtistPtr &artist, QueryMaker::ArtistMatchBehaviour behaviour )
{
    for( QueryMaker *b : builders )
        b->addMatch( artist, behaviour );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    for( QueryMaker *b : builders )
        b->addMatch( album );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    for( QueryMaker *b : builders )
        b->addMatch( genre );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    for( QueryMaker *b : builders )
        b->addMatch( composer );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const Meta::YearPtr &year )
{
    for( QueryMaker *b : builders )
        b->addMatch( year );
    return this;
}

QueryMaker*
MetaQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    for( QueryMaker *b : builders )
        b->addMatch( label );
    return this;
}

QueryMaker*
MetaQueryMaker::limitMaxResultSize( int size )
{
    for( QueryMaker *b : builders )
        b->limitMaxResultSize( size );
    return this;
}

QueryMaker*
MetaQueryMaker::beginAnd()
{
    for( QueryMaker *b : builders )
        b->beginAnd();
    return this;
}

QueryMaker*
MetaQueryMaker::beginOr()
{
    for( QueryMaker *b : builders )
        b->beginOr();
    return this;
}

QueryMaker*
MetaQueryMaker::endAndOr()
{
    for( QueryMaker *b : builders )
        b->endAndOr();
    return this;
}

QueryMaker*
MetaQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    for( QueryMaker *qm : builders )
        qm->setAlbumQueryMode( mode );
    return this;
}

QueryMaker*
MetaQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
    for( QueryMaker *qm : builders )
        qm->setLabelQueryMode( mode );
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
        Q_EMIT queryDone();
    }
    else
        m_queryDoneCountMutex.unlock();
}


