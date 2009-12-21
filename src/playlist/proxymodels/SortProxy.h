/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef AMAROK_PLAYLISTSORTPROXY_H
#define AMAROK_PLAYLISTSORTPROXY_H

#include "ProxyBase.h"
#include "SortScheme.h"

namespace Playlist
{

/**
 * A ProxyModel that implements multilevel sorting for the Playlist.
 * This proxy should sit above the FilterProxy and below the GroupingProxy.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortProxy : public ProxyBase
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    explicit SortProxy( AbstractModel *belowModel, QObject *parent = 0 );

    /**
     * Destructor.
     */
    ~SortProxy();

    /**
     * Comparison function used by sort(). It wraps around a common LessThan-style functor
     * that could be used with any Qt sort algorithm, which implements a multilevel less
     * than comparison.
     * @param left the first index to compare.
     * @param right the second index to compare.
     * @return true if left is to be placed before right, false otherwise.
     */
    bool lessThan( const QModelIndex & left, const QModelIndex & right ) const;

public slots:
    /**
     * Applies a sorting scheme to the playlist.
     * @param scheme the sorting scheme that will be applied.
     */
    void updateSortMap( SortScheme scheme );

    /**
     * Resets the proxy to its original pass-through state.
     */
    void resetSorting();

protected:
    /**
     * Converts a row index that's valid in the proxy below this one to a row index valid
     * in this proxy, with sanity checks.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in this proxy.
     */
    virtual int rowFromSource( int row ) const;

    /**
     * Converts a row index that's valid in this proxy to a row index valid in the proxy
     * below this one, with sanity checks.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in the proxy below this one.
     */
    virtual int rowToSource( int row ) const;

protected slots:
    /**
     * Reapplies the current sorting scheme.
     */
    void invalidateSorting();

private:
    SortScheme m_scheme;               //! The current sorting scheme.
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTPROXY_H
