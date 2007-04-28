/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "metaquerybuilder.h"

#include <QtGlobal>

MetaQueryBuilder::MetaQueryBuilder( const QList<Collection*> &collections )
    : QueryMaker()
    , m_queryDoneCount( 0 )
    , m_queryDoneCountMutex()
{
    foreach( Collection *c, collections )
    {
        QueryMaker *b = c->queryBuilder();
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

MetaQueryBuilder::~MetaQueryBuilder()
{
    foreach( QueryMaker *b, builders )
        delete b;
}

QueryMaker*
MetaQueryBuilder::reset()
{
    m_queryDoneCount = 0;
    foreach( QueryMaker *b, builders )
        b->reset();
    return this;
}

void
MetaQueryBuilder::run()
{
    foreach( QueryMaker *b, builders )
        b->run();
}

void
MetaQueryBuilder::abortQuery()
{
    foreach( QueryMaker *b, builders )
        b->abortQuery();
}

QueryMaker*
MetaQueryBuilder::startTrackQuery()
{
    foreach( QueryMaker *b, builders )
        b->startTrackQuery();
    return this;
}

QueryMaker*
MetaQueryBuilder::startArtistQuery()
{
    foreach( QueryMaker *b, builders )
        b->startArtistQuery();
    return this;
}

QueryMaker*
MetaQueryBuilder::startAlbumQuery()
{
    foreach( QueryMaker *b, builders )
        b->startAlbumQuery();
    return this;
}

QueryMaker*
MetaQueryBuilder::startGenreQuery()
{
    foreach( QueryMaker *b, builders )
        b->startGenreQuery();
    return this;
}

QueryMaker*
MetaQueryBuilder::startComposerQuery()
{
    foreach( QueryMaker *b, builders )
        b->startComposerQuery();
    return this;
}

QueryMaker*
MetaQueryBuilder::startYearQuery()
{
    foreach( QueryMaker *b, builders )
        b->startYearQuery();
    return this;
}

QueryMaker*
MetaQueryBuilder::startCustomQuery()
{
    foreach( QueryMaker *b, builders )
        b->startCustomQuery();
    return this;
}

QueryMaker*
MetaQueryBuilder::addReturnValue( qint64 value )
{
    foreach( QueryMaker *b, builders )
        b->addReturnValue( value );
    return this;
}

QueryMaker*
MetaQueryBuilder::orderBy( qint64 value, bool descending )
{
    foreach( QueryMaker *b, builders )
        b->orderBy( value, descending );
    return this;
}

QueryMaker*
MetaQueryBuilder::includeCollection( const QString &collectionId )
{
    foreach( QueryMaker *b, builders )
        b->includeCollection( collectionId );
    return this;
}

QueryMaker*
MetaQueryBuilder::excludeCollection( const QString &collectionId )
{
    foreach( QueryMaker *b, builders )
        b->excludeCollection( collectionId );
    return this;
}

QueryMaker*
MetaQueryBuilder::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    foreach( QueryMaker *b, builders )
        b->addFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
MetaQueryBuilder::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    foreach( QueryMaker *b, builders )
        b->excludeFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
MetaQueryBuilder::addMatch( const TrackPtr &track )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( track );
    return this;
}

QueryMaker*
MetaQueryBuilder::addMatch( const ArtistPtr &artist )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( artist );
    return this;
}

QueryMaker*
MetaQueryBuilder::addMatch( const AlbumPtr &album )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( album );
    return this;
}

QueryMaker*
MetaQueryBuilder::addMatch( const GenrePtr &genre )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( genre );
    return this;
}

QueryMaker*
MetaQueryBuilder::addMatch( const ComposerPtr &composer )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( composer );
    return this;
}

QueryMaker*
MetaQueryBuilder::addMatch( const YearPtr &year )
{
    foreach( QueryMaker *b, builders )
        b->addMatch( year );
    return this;
}

QueryMaker*
MetaQueryBuilder::addMatch( const DataPtr &data )
{
    DataPtr tmp = const_cast<DataPtr&>( data );
    foreach( QueryMaker *b, builders )
        tmp->addMatchTo( b );
    return this;
}

QueryMaker*
MetaQueryBuilder::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    foreach( QueryMaker *b, builders )
        b->returnResultAsDataPtrs( resultAsDataPtrs );
    return this;
}

QueryMaker*
MetaQueryBuilder::limitMaxResultSize( int size )
{
    foreach( QueryMaker *b, builders )
        b->limitMaxResultSize( size );
    return this;
}

void
MetaQueryBuilder::slotQueryDone()
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

#include "metaquerybuilder.moc"
