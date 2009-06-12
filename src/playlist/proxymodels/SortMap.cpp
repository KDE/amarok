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

#include "SortMap.h"

#include "Debug.h"
#include "FilterProxy.h"
#include "SortProxy.h"

#include <QPair>

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
template< class T >
void
kAdaptiveStableSort( T begin, T end, Playlist::MultilevelLessThan lessThan)
{
    for( T i = begin + 1; i != end; ++i )
        for( T j = i; j != begin && lessThan( *j, *( j - 1 ) ); --j )
            qSwap( *j, *( j - 1 ) );
    /* move version:
    for( int j = 2; j <= n; j++ )
    {
        int i = j - 1;
        int t = list[j];
        while( t < list[i] )
        {
            list[ i + 1 ] = list[ i ];
            i++;
        }
        list[ i + 1 ] = t;
    }*/
    /* pseudopascal
    procedure Straight Insertion Sort (X, n):
    X[0] := − ∞;
    for j := 2 to n do
    begin
        i := j − 1;
        t := X[j];
        while t < X[i] do
        begin
            X[i + 1] := X[i];
            i := i + 1
        end;
        X[i + 1] := t;
    end;
    */
}

namespace Playlist
{

SortMap::SortMap( FilterProxy *sourceProxy )
    : m_sorted( 0 )
{
    DEBUG_BLOCK
    m_sourceProxy = sourceProxy;    //FilterProxy::instance();
    m_rowCount = m_sourceProxy->rowCount();
    m_map = new QList< int >();
    for( int i = 0; i < m_rowCount; i++ )   //defining an identity
        m_map->append( i );     //this should give me amortized O(1)
}

SortMap::~SortMap()
{
    delete m_map;
}

int
SortMap::inv( int proxyRow )
{
    return m_map->indexOf( proxyRow );
}

int
SortMap::map( int sourceRow )
{
    debug() << "SortMap: map sourceRow=" << sourceRow;
    return m_map->value( sourceRow );   //note that if sourceRow>= size(), bad things will happen.
}

void
SortMap::sort( SortScheme *scheme )
{
    debug()<< "about to call qStableSort()";
    //FIXME: Really unclean solution.
    //       As qStableSort() swaps values rather than setting them, it only sorts nicely
    //       when starting from an identity function. This behavior *must* be fixed, but
    //       until then we just use a very inefficient workaround and make sure that
    //       qStableSort() always starts from an identity map.
    //       This preloading takes O(n).
    //                      -- Téo
    m_map->clear();
    for( int i = 0; i < m_rowCount; i++ )   //defining an identity
        m_map->append( i );     //this should give me amortized O(1)
    //qStableSort() is a merge sort and takes Θ(n*logn) in any case.
    qStableSort( m_map->begin(), m_map->end(), MultilevelLessThan( m_sourceProxy, scheme) );
    m_sorted = 1;

    debug() << "Behold the mighty sorting map spamming your terminal:";
    debug() << "  source  sortProxy";   //column headers
    for( int i = 0; i < m_map->length(); i++ )
    {
        debug() << "   " << i << "   " << m_map->value( i );
    }
}

void
SortMap::insertRows( int startRowInSource, int endRowInSource )
{
    //TODO: implement adding
    debug() << "Here I should be adding to the map sourceRows from " << startRowInSource << " to " << endRowInSource;
    m_sorted = 0;   //inserting rows surely invalidates the sorting.
    m_rowCount = m_sourceProxy->rowCount();
}

void
SortMap::deleteRows( int startRowInSource, int endRowInSource )
{
    DEBUG_BLOCK
    debug() << "Removing from the map sourceRows from " << startRowInSource << " to " << endRowInSource;
    QList< int >::iterator startIterator = m_map->begin() + startRowInSource;
    QList< int >::iterator endIterator = m_map->begin() + endRowInSource + 1;   //erase() wants the first item that *won't* be deleted as the end row
    m_map->erase( startIterator, endIterator );     //yay this should take O(1)
    m_rowCount = m_sourceProxy->rowCount();
    // Let's now restore codomain elements consistency.
    // We have to make it so that the set of keys has the same elements as the set of values,
    // and the existing order has to be preserved.
    //FIXME: Really unclean solution.
    //       To restore elements consistency after performing an insertion/deletion of tracks
    //       without sorting everything again I would need to keep track of the codomain
    //       indexes (list values) and correct them. There might be another data structure
    //       that I could use instead of a QList< int > that would possibly allow me to keep
    //       the consistency of the indexes, but I need to do further research.
    //       In the meantime, I'll use a quick, dirty and a bit inefficient solution and
    //       resort everything. To do that, I first need to clear the existing sorting map
    //       and apply an identity map (see SortMap::sort()). After that I'll use a special
    //       adaptive sorting algorithm (kAdaptiveStableSort()).
    //       We will just suppose for now that the user won't add/remove many tracks all at
    //       once.
    //                      -- Téo

    //With a small enough input and a low number of inversions, this sorting is as close to
    //O(n) as it gets.
    QList< QPair< int, int > > *tempConsistencyMap = new QList< QPair< int, int > >();
    //I copy the contents of m_map in a QList< QPair< int, int > >
    {
        QList< int >::iterator it = m_map->begin();
        for( int i = 0; i < m_rowCount; i++ )   //defining an identity
        {
            tempConsistencyMap->append( QPair< int, int >( i, *it ) );     //this should give me amortized O(1)
            ++it;
        }
    }

    //I do a normal stable adaptive sort.
    for( QList< QPair< int, int > >::iterator i = tempConsistencyMap->begin() + 1; i != tempConsistencyMap->end(); ++i )
        for( QList< QPair< int, int > >::iterator j = i; j != tempConsistencyMap->begin() && j->second < ( j - 1 )->second; --j )
            qSwap( *j, *( j - 1 ) );

    //I clear the second column with an identity.
    {
        int j = 0;
        for( QList< QPair< int, int > >::iterator i = tempConsistencyMap->begin(); i != tempConsistencyMap->end(); ++i )
        {
            i->second = j;
            ++j;
        }
    }

    //Finally I sort again to obtain a coherent sortMap.
    for( QList< QPair< int, int > >::iterator i = tempConsistencyMap->begin() + 1; i != tempConsistencyMap->end(); ++i )
        for( QList< QPair< int, int > >::iterator j = i; j != tempConsistencyMap->begin() && j->first < ( j - 1 )->first; --j )
            qSwap( *j, *( j - 1 ) );

    //And I copy it over to m_map.
    m_map->clear();
    for( QList< QPair< int, int > >::iterator i = tempConsistencyMap->begin(); i != tempConsistencyMap->end(); ++i )
        m_map->append( i->second );     //this should give me amortized O(1)

    debug() << "Map consistency restored.";
    debug() << "  source  sortProxy";   //column headers
    for( int i = 0; i < m_map->length(); i++ )
    {
        debug() << "   " << i << "   " << m_map->value( i );
    }
    /*
    I get an inconsistent n-tuple of codomain indexes, such as (5 2 3)
    Why not just do a normal nlogn sort, but this time it's a simple sort that doesn't use
    MultilevelLessThan() but just gives
    1   5
    2   2
    3   4
    4   1
    5   3
    ==>
    1   5
    2   2
    3   3
    sort wrt column II==>
    2   1   2
    3   2   3
    1   3   5
    remove third column and put identity as second==>
    2   1
    3   2
    1   3
    sort wrt column I==>
    1   3   5
    2   1   2
    3   2   3
    ==> return column II
    */
    m_sorted = 1;
}

bool
MultilevelLessThan::operator()(int rowA, int rowB)
{
    quint8 verdict;  //0 = false  1 = true  2 = nextIteration
    for( int i = 0; i < m_scheme->length(); i++ )
    {
        int currentCategory = m_scheme->level( i ).category();  //see enum Column in PlaylistDefines.h
        QVariant dataA = m_sourceProxy->index( rowA, currentCategory ).data();  //FIXME: are you sure you need to do comparisons on sourceProxy indexes?
        QVariant dataB = m_sourceProxy->index( rowB, currentCategory ).data();  //or better, are you sure those rowA and rowB don't need a rowToSource around them?
        if( m_scheme->level( i ).isString() )
        {
            if( dataA.toString() < dataB.toString() )
                verdict = 1;
            else if( dataA.toString() > dataB.toString() )
                verdict = 0;
            else
                verdict = 2;
        }
        else //if it's not a string ==> it's a number
        {
            if( dataA.toInt() < dataB.toInt() )
                verdict = 1;
            else if( dataA.toInt() > dataB.toInt() )
                verdict = 0;
            else
                verdict = 2;
        }
        if( verdict != 2 )
        {
            if( m_scheme->level( i ).order() == Qt::AscendingOrder )
                verdict = verdict ? 0 : 1;
            break;
        }
    }
    return static_cast<bool>( verdict );
}

}   //namespace Playlist
