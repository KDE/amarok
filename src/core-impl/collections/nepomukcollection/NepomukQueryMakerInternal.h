/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#ifndef NEPOMUKQUERYMAKERINTERNAL_H
#define NEPOMUKQUERYMAKERINTERNAL_H

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "NepomukCollection.h"

#include <QObject>
#include <QWeakPointer>


class CustomReturnFunction;
class CustomReturnValue;
class MemoryFilter;

namespace Collections
{

class NepomukCollection;

// Completely inspired by MemoryMakerInternal, for its advantages and completeness.

class NepomukQueryMakerInternal : public QObject
{
    Q_OBJECT
public:

    NepomukQueryMakerInternal( NepomukCollection *collection );
    ~NepomukQueryMakerInternal();

    void runQuery();
    void handleResult();
    void handleResult( const Meta::TrackList &tracks );

//    void setMatchers( MemoryMatcher *matchers );
    void setFilters( MemoryFilter *filters );
    void setMaxSize( int maxSize );
    void setReturnAsDataPtrs( bool returnAsDataPtrs );
    void setType( QueryMaker::QueryType );
    void setCustomReturnFunctions( const QList<CustomReturnFunction*> &functions );
    void setCustomReturnValues( const QList<CustomReturnValue*> &values );

    void setAlbumQueryMode( Collections::QueryMaker::AlbumQueryMode mode )
    {
        m_albumQueryMode = mode;
    }

    void setArtistQueryMode( Collections::QueryMaker::ArtistQueryMode mode )
    {
        m_artistQueryMode = mode;
    }

    void setOrderDescending( bool orderDescending )
    {
        m_orderDescending = orderDescending;
    }

    void setOrderByNumberField( bool orderByNumberField )
    {
        m_orderByNumberField = orderByNumberField;
    }

    void setOrderByField( qint64 orderByField )
    {
        m_orderByField = orderByField;
    }

    void setCollectionId( const QString &collectionId )
    {
        m_collectionId = collectionId;
    }

    void setLabelQueryMode( Collections::QueryMaker::LabelQueryMode mode )
    {
        m_labelQueryMode = mode;
    }

signals:
    void newResultReady( Meta::TrackList );
    void newResultReady( Meta::ArtistList );
    void newResultReady( Meta::AlbumList );
    void newResultReady( Meta::GenreList );
    void newResultReady( Meta::ComposerList );
    void newResultReady( Meta::YearList );
    void newResultReady( QStringList );
    void newResultReady( Meta::LabelList );
    void newResultReady( Meta::DataList );

private:
    template <class PointerType>
    void emitProperResult( const QList<PointerType > &list );

private:
    NepomukCollection * m_collection;
//    MemoryMatcher *m_matchers;
    MemoryFilter *m_filters;
    int m_maxSize;
    bool m_returnAsDataPtrs;
    Collections::QueryMaker::QueryType m_type;
    Collections::QueryMaker::AlbumQueryMode m_albumQueryMode;
    Collections::QueryMaker::ArtistQueryMode m_artistQueryMode;
    Collections::QueryMaker::LabelQueryMode m_labelQueryMode;
    bool m_orderDescending;
    bool m_orderByNumberField;
    qint64 m_orderByField;
    QString m_collectionId;
    QList<CustomReturnFunction*> m_returnFunctions;
    QList<CustomReturnValue*> m_returnValues;

};

}

#endif // NEPOMUKQUERYMAKERINTERNAL_H
