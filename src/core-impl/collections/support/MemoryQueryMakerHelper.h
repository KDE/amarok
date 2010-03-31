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

#ifndef MEMORYQUERYMAKERHELPER_H
#define MEMORYQUERYMAKERHELPER_H

#include <QList>
#include "core/meta/Meta.h"

namespace Collections {

namespace MemoryQueryMakerHelper
{
    template <class PointerType>
    QList<PointerType > orderListByName( const QList<PointerType > &list, bool descendingOrder );

    Meta::YearList orderListByYear ( const Meta::YearList &list, bool descendingOrder );

    Meta::TrackList orderListByString( const Meta::TrackList &tracks, qint64 value, bool orderDescending );
    Meta::TrackList orderListByNumber( const Meta::TrackList &tracks, qint64 value, bool orderDescending );

    template<typename T>
    QList<T> reverse(const QList<T> &l);
}

} //namespace Collections

#endif
