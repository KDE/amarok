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

#include "NepomukQueryMaker.h"
#include "NepomukCollection.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

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
        	/*
            QString query = m_queryMaker->query();
            QStringList result = m_queryMaker->runQuery( query );
            if( !m_aborted )
                m_queryMaker->handleResult( result );
            setFinished( !m_aborted );
            */
        }
    private:
        NepomukQueryMaker *m_queryMaker;

        bool m_aborted;
};

NepomukQueryMaker::NepomukQueryMaker()
{
    reset();
}

NepomukQueryMaker::~NepomukQueryMaker()
{
}

QueryMaker*
NepomukQueryMaker::reset()
{
    return this;
}

void
NepomukQueryMaker::abortQuery()
{
}

void
NepomukQueryMaker::run()
{

}

QueryMaker*
NepomukQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
	Q_UNUSED( resultAsDataPtrs )
    return this;
}



QueryMaker*
NepomukQueryMaker::startTrackQuery()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::startArtistQuery()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::startAlbumQuery()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::startGenreQuery()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::startComposerQuery()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::startYearQuery()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::startCustomQuery()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::includeCollection( const QString &collectionId )
{
	Q_UNUSED( collectionId )
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeCollection( const QString &collectionId )
{
	Q_UNUSED( collectionId )
	return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const TrackPtr &track )
{
	Q_UNUSED( track )
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const ArtistPtr &artist )
{
	Q_UNUSED( artist )
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const AlbumPtr &album )
{
	Q_UNUSED( album )
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const GenrePtr &genre )
{
	Q_UNUSED( genre )
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const ComposerPtr &composer )
{
	Q_UNUSED( composer )
	return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const YearPtr &year )
{
	Q_UNUSED( year )
	return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const DataPtr &data )
{
	Q_UNUSED( data )
    return this;
}

QueryMaker*
NepomukQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( matchBegin )
	Q_UNUSED( matchEnd )
	return this;
}

QueryMaker*
NepomukQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( matchBegin )
	Q_UNUSED( matchEnd )
	return this;
}

QueryMaker*
NepomukQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( compare )
	return this;
}

QueryMaker*
NepomukQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
	Q_UNUSED( value )
	Q_UNUSED( filter )
	Q_UNUSED( compare )
	return this;
}

QueryMaker*
NepomukQueryMaker::addReturnValue( qint64 value )
{
	Q_UNUSED( value )
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
	Q_UNUSED( function )
	Q_UNUSED( value )
    return this;
}

QueryMaker*
NepomukQueryMaker::orderBy( qint64 value, bool descending )
{
	Q_UNUSED( value )
	Q_UNUSED( descending )
    return this;
}

QueryMaker*
NepomukQueryMaker::limitMaxResultSize( int size )
{
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
    return this;
}

QueryMaker*
NepomukQueryMaker::beginOr()
{
    return this;
}

QueryMaker*
NepomukQueryMaker::endAndOr()
{
    return this;
}

#include "NepomukQueryMaker.moc"
