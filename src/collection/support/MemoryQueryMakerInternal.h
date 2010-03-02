/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef MEMORYQUERYMAKERINTERNAL_H
#define MEMORYQUERYMAKERINTERNAL_H

#include "collection/QueryMaker.h"
#include "meta/Meta.h"

#include <QObject>
#include <QPointer>
#include <QWeakPointer>

class CustomReturnFunction;
class CustomReturnValue;
class MemoryCollection;
class MemoryFilter;
class MemoryMatcher;

/**
  * A helper class for MemoryQueryMaker
  * This class will run in a dedicated thread. It exists so the actual MemoryQueryMaker
  * can be safely deleted in the original thread while the query is still running.
  * All relevant information is passed from MemoryQueryMaker to MemoryQueryMakerInternal
  * using the provided setter methods.
  */
class MemoryQueryMakerInternal : public QObject
{
    Q_OBJECT
public:
    /**
      * Creates a new MemoryQueryMakerInternal that will query collection.
      * This class will run in a dedicated thread. It exists so the actual MemoryQueryMaker
      * can be safely deleted in the original thread while the query is still running.
      * @param guard a class that will be deleted before collection. It is used to
      * ensure that MemoryQueryMakerInternal does not access a dangling MemoryCollection
      * pointer.
      * @param collection the MemoryCollection instance that the query should be run on.
      */
    MemoryQueryMakerInternal( const QWeakPointer<MemoryCollection> &collection );
    ~MemoryQueryMakerInternal();


    void runQuery();
    void handleResult();
    void handleResult( const Meta::TrackList &tracks );

    void setMatchers( MemoryMatcher *matchers );
    void setFilters( MemoryFilter *filters );
    void setRandomize( bool randomize );
    void setMaxSize( int maxSize );
    void setReturnAsDataPtrs( bool returnAsDataPtrs );
    void setType( QueryMaker::QueryType );
    void setCustomReturnFunctions( const QList<CustomReturnFunction*> &functions );
    void setCustomReturnValues( const QList<CustomReturnValue*> &values );
    void setAlbumQueryMode( QueryMaker::AlbumQueryMode mode ) { m_albumQueryMode = mode; }
    void setOrderDescending( bool orderDescending ) { m_orderDescending = orderDescending; }
    void setOrderByNumberField( bool orderByNumberField ) { m_orderByNumberField = orderByNumberField; }
    void setOrderByField( qint64 orderByField ) { m_orderByField = orderByField; }
    void setCollectionId( const QString &collectionId ) { m_collectionId = collectionId; }

signals:
    void newResultReady( QString collectionId, Meta::TrackList );
    void newResultReady( QString collectionId, Meta::ArtistList );
    void newResultReady( QString collectionId, Meta::AlbumList );
    void newResultReady( QString collectionId, Meta::GenreList );
    void newResultReady( QString collectionId, Meta::ComposerList );
    void newResultReady( QString collectionId, Meta::YearList );
    void newResultReady( QString collectionId, Meta::DataList );
    void newResultReady( QString collectionId, QStringList );

private:
    template <class PointerType>
    void emitProperResult( const QList<PointerType > &list );

private:
    QWeakPointer<MemoryCollection> m_collection;
    QPointer<QObject> m_guard;
    MemoryMatcher *m_matchers;
    MemoryFilter *m_filters;
    bool m_randomize;
    int m_maxSize;
    bool m_returnAsDataPtrs;
    QueryMaker::QueryType m_type;
    QueryMaker::AlbumQueryMode m_albumQueryMode;
    bool m_orderDescending;
    bool m_orderByNumberField;
    qint64 m_orderByField;
    QString m_collectionId;
    QList<CustomReturnFunction*> m_returnFunctions;
    QList<CustomReturnValue*> m_returnValues;
};

#endif
