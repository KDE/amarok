/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
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

#ifndef AMAROK_ABSTRACTMODEL_H
#define AMAROK_ABSTRACTMODEL_H

#include "meta/Meta.h"

#include <QAbstractItemModel>

namespace Playlist
{

/**
 * An abstract base class that defines a common interface of any playlist model.
 * Members declared here must be implemented by Playlist::Model and all proxies.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class AbstractModel
{
public:
    /**
     * Returns the currently active row, translated to proxy rows
     * (or -1 if the current row is not represented by this proxy).
     * @return The currently active (playing) row in proxy terms.
     */
    virtual int activeRow() const = 0;

    virtual int columnCount( const QModelIndex& ) const = 0;

    /**
     * Clears the current search term.
     */
    virtual void clearSearchTerm() = 0;

    /**
     * Get the current search fields bitmask.
     * @return The current search fields.
     */
    virtual int currentSearchFields() = 0;

    /**
     * Get the current search term.
     * @return The curent search term.
     */
    virtual QString currentSearchTerm() = 0;

    /**
     * Forwards the request down the proxy stack and gets the data at an index.
     * @param index the index for which to retrieve the data from the model.
     * @return the data from the model.
     */
    virtual QVariant data( const QModelIndex& index, int role ) const = 0;

    /**
     * Handles the data supplied by a drag and drop operation that ended with the given
     * action.
     * @param data the MIME data.
     * @param action the drop action of the current drag and drop operation.
     * @param row the row where the operation ended.
     * @param column the column where the operation ended.
     * @param parent the parent index.
     * @return true if the data and action can be handled by the model; otherwise false.
     */
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) = 0;

    /**
     * Forwards a search down through the stack of ProxyModels.
     * Find the first track in the playlist that matches the search term in one of the
     * specified search fields. Playlist::Model::find() emits found() or notFound() depending on
     * whether a match is found.
     * @param searchTerm The term to search for.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match, -1 if no match is found.
     */
    virtual int find( const QString &searchTerm, int searchFields ) = 0;

    /**
     * Forwards through the stack of ProxyModels a top to bottom search for the next item.
     * Find the first track below a given row that matches the search term in one of the
     * specified search fields. Playlist::Model::findNext() emits found() or notFound() depending on
     * whether a match is found. If no row is found below the current row, the function wraps
     * around and returns the first match. If no match is found at all, -1 is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match below the offset, -1 if no match is found.
     */
    virtual int findNext( const QString &searchTerm, int selectedRow, int searchFields ) = 0;

    /**
     * Forwards through the stack of ProxyModels a bottom to top search for the next item.
     * Find the first track above a given row that matches the search term in one of the
     * specified search fields. Playlist::Model::findPrevious() emits found() or notFound() depending on
     * whether a match is found. If no row is found above the current row, the function wraps
     * around and returns the last match. If no match is found at all, -1 is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match above the offset, -1 if no match is found.
     */
    virtual int findPrevious( const QString &searchTerm, int selectedRow, int searchFields ) = 0;

    /**
     * Returns the item flags for the given index.
     * @param index the index to retrieve the flags for.
     * @return the item flags.
     */
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const = 0;

    /**
     * Returns an object that contains serialized items of data corresponding to the list of indexes specified.
     * @param indexes a list of indexes.
     * @return the MIME data corresponding to the indexes.
     */
    virtual QMimeData* mimeData( const QModelIndexList &indexes ) const = 0;

    /**
     * Returns a list of MIME types that can be used to describe a list of model indexes.
     * @return a QStringList of MIME types.
     */
    virtual QStringList mimeTypes() const = 0;

    /**
     * Forwards the number of rows from the FilterProxy as SortProxy by definition shouldn't
     * change the row count.
     * @param parent the parent of the rows to count.
     * @return the number of rows.
     */
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const = 0;

    /**
     * Checks if a row exists in the current model or proxy.
     * @param row the row in the model or proxy.
     * @return true is the row exists, otherwise false.
     */
    virtual bool rowExists( int row ) const = 0;

    /**
     * Sets the currently active (playing) row, translated for this proxy.
     * @param row the row to be set as active.
     */
    virtual void setActiveRow( int row ) = 0;

    /**
     * Returns the drop actions supported by this model.
     * @return the drop actions.
     */
    virtual Qt::DropActions supportedDropActions() const = 0;

    /**
     * Asks the model sitting below the total length of the playlist.
     * @return the total length of the playlist.
     */
    virtual int totalLength() const = 0;

    /**
     * Returns a pointer to the track at a given row.
     * @param row the row to return the track pointer for.
     * @return a pointer to the track at the given row.
     */
    virtual Meta::TrackPtr trackAt( int row ) const = 0;

    /**
     * Destructor.
     */
    virtual ~AbstractModel() {};
};

}   //namespace Playlist

#endif  //AMAROK_ABSTRACTMODEL_H
