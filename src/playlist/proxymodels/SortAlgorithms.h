/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#ifndef AMAROK_SORTALGORITHMS_H
#define AMAROK_SORTALGORITHMS_H

#include "ProxyBase.h"
#include "SortScheme.h"

namespace Playlist
{

/**
 * This struct defines a comparison functor that can be used by qSort(), qStableSort(), or
 * other sorting algorithms with a similar interface.
 * The comparison is operated on multiple levels of a Playlist::SortScheme.
 * @warning This functor is specific for this particular problem and wouldn't probably do
 * any good for sorting anything else than tracks.
 * @author Téo Mrnjavac <teo@kde.org>
 */
struct multilevelLessThan
{
    /**
     * Constructor.
     */
    multilevelLessThan()
        : m_scheme( SortScheme() )
    { }

    /**
     * Set sort scheme
     * @param scheme the sorting scheme that needs to be applied.
     */
    void setSortScheme( const SortScheme & scheme );

    /**
     * Takes two row numbers from the source model and compares the corresponding indexes
     * based on a number of chosen criteria (columns).
     * @param sourceModel the source model.
     * @param sourceModelRowA the first row.
     * @param sourceModelRowB the second row.
     * @return true if sourceModelRowA is to be placed before sourceModelRowB, false otherwise.
     */
    bool operator()( const QAbstractItemModel* sourceModel, int sourceModelRowA, int sourceModelRowB ) const;

    private:
        template<typename T>
        int compareBySortableName( const AmarokSharedPointer<T> &left, const AmarokSharedPointer<T> &right ) const;

        SortScheme m_scheme;    //!< The current sorting scheme.
};

}   //namespace Playlist

#endif  //AMAROK_SORTALGORITHMS_H
