/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef AMAROK_PLAYLISTSORTPROXY_H
#define AMAROK_PLAYLISTSORTPROXY_H

#include "ProxyBase.h"
#include "FilterProxy.h"
#include "SortScheme.h"

namespace Playlist
{

/**
 * A ProxyModel that implements multilevel sorting for the Playlist.
 * This proxy should sit above the FilterProxy and below the GroupingProxy.
 * @author To Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortProxy : public ProxyBase
{
    Q_OBJECT
public:
    /**
     * Returns the single instance of SortProxy.
     * @return the class instance.
     */
    static SortProxy *instance();

    /**
     * Comparison function used by sort(). It wraps around a common LessThan-style functor
     * that could be used with any Qt sort algorithm, which implements a multilevel less
     * than comparison.
     * @param left the first index to compare.
     * @param right the second index to compare.
     * @return true if left is to be placed before right, false otherwise.
     */
    bool lessThan( const QModelIndex & left, const QModelIndex & right ) const;

// Pass-through public methods, basically identical to those in Playlist::FilterProxy, that
// pretty much just forward stuff through the stack of proxies start here.
// Please keep them sorted alphabetically.  -- To

    /**
     * Returns the item flags for the given index.
     * @param index the index to retrieve the flags for.
     * @return the item flags.
     */
    Qt::ItemFlags flags( const QModelIndex& index ) const;

    /**
     * Returns an object that contains serialized items of data corresponding to the list of indexes specified.
     * @param indexes a list of indexes.
     * @return the MIME data corresponding to the indexes.
     */
    QMimeData* mimeData( const QModelIndexList &indexes ) const;

    /**
     * Returns a list of MIME types that can be used to describe a list of model indexes.
     * @return a QStringList of MIME types.
     */
    QStringList mimeTypes() const;

    /**
     * Forwards the number of rows from the FilterProxy as SortProxy by definition shouldn't
     * change the row count.
     * @param parent the parent of the rows to count.
     * @return the number of rows.
     */
    int rowCount( const QModelIndex &parent = QModelIndex() ) const;

    /**
     * Checks if a row exists in the ProxyModel.
     * @param row the row in the Proxy.
     * @return true is the row exists, otherwise false.
     */
    bool rowExists( int row ) const;

    /**
     * Converts a row index that's valid in the proxy below this one to a row index valid
     * in this proxy, with sanity checks.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in this proxy.
     */
    int rowFromSource( int row ) const;

    /**
     * Converts a row index that's valid in this proxy to a row index valid in the proxy
     * below this one, with sanity checks.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in the proxy below this one.
     */
    int rowToSource( int row ) const;

    /**
     * Sets the currently active (playing) row, translated for this proxy.
     * @param row the row to be set as active.
     */
    void setActiveRow( int row );

    /**
     * Returns the drop actions supported by this model.
     * @return the drop actions.
     */
    Qt::DropActions supportedDropActions() const;

    /**
     * Asks the model sitting below the total length of the playlist.
     * @return the total length of the playlist.
     */
    int totalLength() const;

    /**
     * Returns a pointer to the track at a given row.
     * @param row the row to return the track pointer for.
     * @return a pointer to the track at the given row.
     */
    Meta::TrackPtr trackAt( int row ) const;

public slots:
    /**
     * Applies a sorting scheme to the playlist.
     * @param scheme the sorting scheme that will be applied.
     */
    void updateSortMap( SortScheme *scheme );

private:
    /**
     * Constructor.
     */
    SortProxy();

    /**
     * Destructor.
     */
    ~SortProxy();

    static SortProxy *s_instance;       //! Instance member.

    SortScheme *m_scheme;               //! The current sorting scheme.
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTPROXY_H
