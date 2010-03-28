/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "MemoryQueryMakerHelper.h"

#include "collection/support/MemoryCustomValue.h"
#include "core/meta/Meta.h"

#include <QList>
#include <QSet>
#include <QStack>
#include <QtAlgorithms>

#include <KRandomSequence>
#include <KSortableList>

using namespace Collections;

template <class PointerType>
QList<PointerType>
MemoryQueryMakerHelper::orderListByName( const QList<PointerType> &list, bool descendingOrder )
{
    QList<PointerType> resultList = list;
    KSortableList<PointerType, QString> sortList;
    foreach( const PointerType &pointer, list )
    {
        sortList.insert( pointer->name(), pointer );
    }
    sortList.sort();
    QList<PointerType> tmpList;
    typedef KSortableItem<PointerType,QString> SortItem;
    foreach( const SortItem &item, sortList )
    {
       tmpList.append( item.second );
    }
    if( descendingOrder )
    {
        //KSortableList uses qSort, which orders a list in ascending order
        resultList = reverse<PointerType>( tmpList );
    }
    else
    {
        resultList = tmpList;
    }
    return resultList;
}

Meta::YearList
MemoryQueryMakerHelper::orderListByYear( const Meta::YearList &list, bool descendingOrder )
{
    KSortableList<Meta::YearPtr, double> sortList;
    foreach( Meta::YearPtr pointer, list )
    {
        sortList.insert( pointer->name().toDouble(), pointer );
    }
    sortList.sort();
    QList<Meta::YearPtr> tmpList;
    typedef KSortableItem<Meta::YearPtr,double> SortItem;
    foreach( const SortItem &item, sortList )
    {
        tmpList.append( item.second );
    }
    if( descendingOrder )
    {
        //KSortableList uses qSort, which orders a list in ascending order
        return reverse<Meta::YearPtr>( tmpList );
    }
    else
    {
        return tmpList;
    }
}

template<typename T>
QList<T>
MemoryQueryMakerHelper::reverse(const QList<T> &l)
{
    QList<T> ret;
    for (int i=l.size() - 1; i>=0; --i)
        ret.append(l.at(i));
    return ret;
}

Meta::TrackList
MemoryQueryMakerHelper::orderListByString( const Meta::TrackList &tracks, qint64 value, bool orderDescending )
{
    Meta::TrackList resultList = tracks;
    CustomReturnValue *crv = CustomValueFactory::returnValue( value );
    if( crv )
    {
        KSortableList<Meta::TrackPtr, QString> sortList;
        foreach( const Meta::TrackPtr &pointer, tracks )
        {
            sortList.insert( crv->value( pointer ), pointer );
        }
        sortList.sort();
        Meta::TrackList tmpList;
        typedef KSortableItem<Meta::TrackPtr,QString> SortItem;
        foreach( const SortItem &item, sortList )
        {
           tmpList.append( item.second );
        }
        if( orderDescending )
        {
            //KSortableList uses qSort, which orders a list in ascending order
            resultList = reverse<Meta::TrackPtr>( tmpList );
        }
        else
        {
            resultList = tmpList;
        }
    }
    delete crv;
    return resultList;
}

Meta::TrackList
MemoryQueryMakerHelper::orderListByNumber( const Meta::TrackList &tracks, qint64 value, bool orderDescending )
{
    Meta::TrackList resultList = tracks;
    CustomReturnValue *crv = CustomValueFactory::returnValue( value );
    if( crv )
    {
        KSortableList<Meta::TrackPtr, double> sortList;
        foreach( const Meta::TrackPtr &pointer, tracks )
        {
            sortList.insert( crv->value( pointer ).toDouble(), pointer );
        }
        sortList.sort();
        Meta::TrackList tmpList;
        typedef KSortableItem<Meta::TrackPtr,double> SortItem;
        foreach( const SortItem &item, sortList )
        {
           tmpList.append( item.second );
        }
        if( orderDescending )
        {
            //KSortableList uses qSort, which orders a list in ascending order
            resultList = reverse<Meta::TrackPtr>( tmpList );
        }
        else
        {
            resultList = tmpList;
        }
    }
    delete crv;
    return resultList;
}

template QList<Meta::AlbumPtr> MemoryQueryMakerHelper::orderListByName( const QList<Meta::AlbumPtr > &list, bool );
template QList<Meta::ArtistPtr> MemoryQueryMakerHelper::orderListByName( const QList<Meta::ArtistPtr > &list, bool );
template QList<Meta::GenrePtr> MemoryQueryMakerHelper::orderListByName( const QList<Meta::GenrePtr > &list, bool );
template QList<Meta::ComposerPtr> MemoryQueryMakerHelper::orderListByName( const QList<Meta::ComposerPtr > &list, bool );

