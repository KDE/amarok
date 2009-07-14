/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
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
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
struct multilevelLessThan
{
    /**
     * Constructor.
     * @param sourceProxy a pointer to the FilteProxy instance.
     * @param scheme the sorting scheme that needs to be applied.
     */
    multilevelLessThan( QAbstractItemModel *sourceProxy, const SortScheme &scheme )
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
        QAbstractItemModel *m_sourceProxy;     //! The proxy or model which holds the rows that need to be sorted.
        SortScheme m_scheme;           //! The current sorting scheme.
};

}   //namespace Playlist

#endif  //AMAROK_SORTALGORITHMS_H
