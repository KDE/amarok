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

#ifndef NEPOMUKQUERYMAKER_H
#define NEPOMUKQUERYMAKER_H

#include "NepomukArtist.h"

#include "QueryMaker.h"

#include <QStack>
#include <QString>
#include <QUuid>

#include <threadweaver/Job.h>
#include <Soprano/Model>

class Kurl;
class NepomukCollection;
class NepomukWorkerThread;

class NepomukQueryMaker : public QueryMaker
{
    Q_OBJECT
    
	public:
	    NepomukQueryMaker(NepomukCollection *collection, Soprano::Model* model);
	    virtual ~NepomukQueryMaker();
	
	    virtual QueryMaker* reset();
	    virtual void abortQuery();
	    virtual void run();
	
	    virtual QueryMaker* setReturnResultAsDataPtrs( bool resultAsDataPtrs );
	    
        virtual QueryMaker* setQueryType( QueryType type );	
	    virtual QueryMaker* includeCollection( const QString &collectionId );
	    virtual QueryMaker* excludeCollection( const QString &collectionId );
	
	    virtual QueryMaker* addMatch( const Meta::TrackPtr &track );
	    virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist );
	    virtual QueryMaker* addMatch( const Meta::AlbumPtr &album );
	    virtual QueryMaker* addMatch( const Meta::GenrePtr &genre );
	    virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer );
	    virtual QueryMaker* addMatch( const Meta::YearPtr &year );
	    virtual QueryMaker* addMatch( const Meta::DataPtr &data );
	
	    virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );
	    virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );
	
	    virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
	    virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
	
	    virtual QueryMaker* addReturnValue( qint64 value );
	    virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
	    virtual QueryMaker* orderBy( qint64 value, bool descending = false );
        virtual QueryMaker* orderByRandom();
	
	    virtual QueryMaker* limitMaxResultSize( int size );
	
	    //virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
	
	    virtual QueryMaker* beginAnd();
	    virtual QueryMaker* beginOr();
	    virtual QueryMaker* endAndOr();
	
	    //virtual int validFilterMask();
	    
	    // functions only for use in nepo collection

        // for using blocking, defaults to non blocking
        void blocking( bool enabled );
        
        QStringList collectionIds() const;

        Meta::DataList data( const QString &id ) const;
        Meta::TrackList tracks( const QString &id ) const;
        Meta::AlbumList albums( const QString &id ) const;
        Meta::ArtistList artists( const QString &id ) const;
        Meta::GenreList genres( const QString &id ) const;
        Meta::ComposerList composers( const QString &id ) const;
        Meta::YearList years( const QString &id ) const;
        QStringList customData( const QString &id ) const;
        
	    virtual QString buildQuery();
	    virtual void doQuery(const QString& );

	    virtual QueryMaker* addMatch( const KUrl &url);
        virtual QueryMaker* addMatchId( const QString &uid);
	
    public slots:
        void done( ThreadWeaver::Job * job );
	    
	private:

        QString likeCondition( const QString &text, bool matchBegin, bool matchEnd ) const;
        QString andOr() const;

        void addEmptyMatch( const qint64 value, bool optional = false );

        Meta::DataList m_data;
        QueryType m_queryType;
        QString m_queryMatch;
        QString m_queryFilter;
        bool m_resultAsDataPtrs;
        NepomukWorkerThread *m_worker;
        NepomukCollection *m_collection;
        Soprano::Model *m_model;
        QString m_queryOrderBy;
        bool m_blocking;
        int m_queryLimit;
        // used to prevend double use without reseting
        bool m_used;
        QStack<bool> m_andStack;
};

#endif /*NEPOMUKQUERYMAKER_H*/
