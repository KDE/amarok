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

#ifndef AMAROK_PLAYLISTSORTMAP_H
#define AMAROK_PLAYLISTSORTMAP_H

#include "FilterProxy.h"
#include "SortScheme.h"

#include <QVector>
#include <QtAlgorithms>

namespace Playlist
{

/**
 * This class defines a correspondence between source rows and sorted rows, and handles the
 * generation of the correspondence from a Playlist::SortScheme.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortMap
{
public:
    /**
     * Constructor.
     * @param rowCount number of rows to be handled.
     */
    SortMap( FilterProxy *sourceProxy );

    /**
     * Destructor.
     */
    ~SortMap();

    /**
     * Applies the inverse mapping of a row in the proxy to the corresponding row in the
     * source model.
     * @param proxyRow a row in the proxy model.
     * @return the corresponding row in the source model.
     */
    int inv( int proxyRow );

    /**
     * Applies the mapping of a row in the source model to the corresponding row in the
     * proxy.
     * @param sourceRow a row in the source model.
     * @return the corresponding row in the proxy.
     */
    int map( int sourceRow );

    /**
     * Adds a number of newly inserted rows to the map. The map is kept in sync with the
     * proxy but isn't sorted.
     * @param startRowInSource the position of the first row.
     * @param rowCount the number of added rows.
     */
    void insertRows( int startRowInSource, int endRowInSource );

    //NOTE to self by Téo: isn't it true that by just removing rows the sorting should be preserved?
    /**
     * Removes a number of newly deleted rows from the map. The map is kept in sync with the
     * proxy.
     * @param startRowInSource the position of the first row.
     * @param rowCount the number of removed rows.
     */
    void deleteRows( int startRowInSource, int endRowInSource );

    /**
     * Converts a sorting scheme to a permutation of source rows.
     * @param scheme the SortScheme from which to generate the map.
     */
    void sort( SortScheme *scheme );

    /**
     * Checks if the current mapping applies a sorting scheme.
     * @return true if the map is sorted, false otherwise.
     */
    inline bool isSorted(){ return m_sorted; };

private:
    QList< int > *m_map;
    int m_rowCount;
    FilterProxy *m_sourceProxy;
    bool m_sorted;
};

/**
 * Comparison functor used by qSort() or qStableSort().
 */
struct MultilevelLessThan
{
    /**
     * Constructor.
     */
    MultilevelLessThan( FilterProxy *sourceProxy, SortScheme *scheme )
        : m_sourceProxy( sourceProxy )
        , m_scheme( scheme )
        {};
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

#endif  //AMAROK_PLAYLISTSORTMAP_H
