/***************************************************************************
 *   Copyright © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AMAROK_SORTALGORITHMS_H
#define AMAROK_SORTALGORITHMS_H

#include "FilterProxy.h"
#include "SortScheme.h"

#include <QtAlgorithms>

/**
 * Implements straight insertion sort.
 * A sorting algorithm falls into the adaptive sort family if it takes advantage of existing
 * order in its input. Straight insertion sort is adaptive and stable, and it is a variant
 * of the classic insertion sort algorithm.
 * Straight insertion sort takes O(n^2), so it's quite inefficient on large inputs, but if
 * the input list has a high presortedness (hence a low number of inversions), the algorithm
 * can perform noticeably faster than an optimal worst-case O(n*logn) algorithm.
 * @param begin STL-style iterator that points to the beginning of the input container
 * @param end STL-style iterator that points to the end of the input container
 * @param lessThan functor used instead of operator<()
 */
template< typename RandomAccessIterator, typename LessThan >
void
kAdaptiveStableSort( RandomAccessIterator begin, RandomAccessIterator end, LessThan lessThan)
{
    for( RandomAccessIterator i = begin + 1; i != end; ++i )
        for( RandomAccessIterator j = i; j != begin && lessThan( *j, *( j - 1 ) ); --j )
            qSwap( *j, *( j - 1 ) );
}

enum PairElement
{
    first = 0,
    second
};

template< typename T >
struct pairLessThan
{
    /**
     * Constructor.
     * @param position first or second item in the pair.
     */
    pairLessThan( PairElement position )
        : m_position( position )
    {}

    bool operator()( const T &t1, const T &t2 )
    {
        return ( m_position == first ) ? ( t1.first < t2.first ) : ( t1.second < t2.second );
    }

    private:
        PairElement m_position;
};

namespace Playlist
{

/**
 * This struct defines a comparison functor that can be used by qSort(), qStableSort(), or
 * other sorting algorithms with a similar interface.
 * It is specific for this problem and wouldn't probably do any good for sorting anything
 * else than tracks.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
struct multilevelLessThan
{
    /**
     * Constructor.
     * @param sourceProxy a pointer to the FilteProxy instance.
     * @param scheme the sorting scheme that needs to be applied.
     */
    multilevelLessThan( FilterProxy *sourceProxy, SortScheme *scheme )
        : m_sourceProxy( sourceProxy )
        , m_scheme( scheme )
    {}
    
    /**
     * Takes two row numbers from the proxy and compares the corresponding indexes based on
     * a number of chosen criteria (columns).
     * @param rowA the first row.
     * @param rowB the second row.
     * @return true if rowA is to be placed before rowB, false otherwise.
     */
    bool operator()( int rowA, int rowB );
    
    private:
        FilterProxy *m_sourceProxy;
        SortScheme *m_scheme;
};

}   //namespace Playlist

#endif  //AMAROK_SORTALGORITHMS_H