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

#include <QGlobal>

MetaQueryBuilder::MetaQueryBuilder( const QList<Collection*> &collections )
    : super()
    , m_queryDoneCount( 0 )
    , m_queryDoneCountMutex()
{
    foreach( Collection *c, collections )
    {
        QueryBuilder *b = c->queryBuilder();
        builders.append( b );
        connect( b, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ) );
    }
}

MetaQueryBuilder::~MetaQueryBuilder()
{
    foreach( QueryBuilder *b, builders )
        delete b;
}

QueryBuilder*
MetaQueryBuilder::reset()
{
    m_queryDoneCount = 0;
    foreach( QueryBuilder *b, builders )
        b->reset();
    return this;
}

QueryBuilder*
MetaQueryBuilder::run()
{
    foreach( QueryBuilder *b, builders )
        b->run();
    return this;
}

QueryBuilder*
MetaQueryBuilder::abortQuery()
{
    foreach( QueryBuilder *b, builders )
        b->abortQuery();
    return this;
}

QueryBuilder*
MetaQueryBuilder::startTrackQuery()
{
    foreach( QueryBuilder *b, builders )
        b->startTrackQuery();
    return this;
}

QueryBuilder*
MetaQueryBuilder::startArtistQuery()
{
    foreach( QueryBuilder *b, builders )
        b->startArtistQuery();
    return this;
}

QueryBuilder*
MetaQueryBuilder::startAlbumQuery()
{
    foreach( QueryBuilder *b, builders )
        b->startAlbumQuery();
    return this;
}

QueryBuilder*
MetaQueryBuilder::startGenreQuery()
{
    foreach( QueryBuilder *b, builders )
        b->startGenreQuery();
    return this;
}

QueryBuilder*
MetaQueryBuilder::startComposerQuery()
{
    foreach( QueryBuilder *b, builders )
        b->startComposerQuery();
    return this;
}

QueryBuilder*
MetaQueryBuilder::startYearQuery()
{
    foreach( QueryBuilder *b, builders )
        b->startYearQuery();
    return this;
}

QueryBuilder*
MetaQueryBuilder::addReturnValue( qint64 value )
{
    foreach( QueryBuilder *b, builders )
        b->addReturnValue( value );
    return this;
}

QueryBuilder*
MetaQueryBuilder::orderBy( qint64 value, bool descending = false )
{
    foreach( QueryBuilder *b, builders )
        b->orderBy( value, descending );
    return this;
}

QueryBuilder*
MetaQueryBuilder::includeCollection( const QString &collectionId )
{
    foreach( QueryBuilder *b, builders )
        b->includeCollection( collectionId );
    return this;
}

QueryBuilder*
MetaQueryBuilder::excludeCollection( const QString &collectionId )
{
    foreach( QueryBuilder *b, builders )
        b->excludeCollection( collectionid );
    return this;
}

QueryBuilder*
MetaQueryBuilder::addFilter( qint64 value, const QString &filter )
{
    foreach( QueryBuilder *b, builders )
        b->addFilter( value, filter );
    return this;
}

QueryBuilder*
MetaQueryBuilder::excludeFilter( qint64 value, const QString &filter )
{
    foreach( QueryBuilder *b, builders )
        b->excludeFilter( value, filter );
    return this;
}

QueryBuilder*
MetaQueryBuilder::addMatch( const TrackPtr &track )
{
    foreach( QueryBuilder *b, builders )
        b->addMatch( track );
    return this;
}

QueryBuilder*
MetaQueryBuilder::addMatch( const ArtistPtr &artist )
{
    foreach( QueryBuilder *b, builders )
        b->addMatch( artist );
    return this;
}

QueryBuilder*
MetaQueryBuilder::addMatch( const ALbumPtr &album )
{
    foreach( QueryBuilder *b, builders )
        b->addMatch( album );
    return this;
}

QueryBuilder*
MetaQueryBuilder::addMatch( const GenrePtr &genre )
{
    foreach( QueryBuilder *b, builders )
        b->addMatch( genre );
    return this;
}

QueryBuilder*
MetaQueryBuilder::addMatch( const ComposerPtr &composer )
{
    foreach( QueryBuilder *b, builders )
        b->addMatch( composer );
    return this;
}

QueryBuilder*
MetaQueryBuilder::addMatch( const YearPtr &year )
{
    foreach( QueryBuilder *b, builders )
        b->addMatch( year );
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
