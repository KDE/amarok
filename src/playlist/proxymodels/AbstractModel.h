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

#ifndef AMAROK_PLAYLISTABSTRACTMODEL_H
#define AMAROK_PLAYLISTABSTRACTMODEL_H

#include "meta/Meta.h"
#include "playlist/PlaylistDefines.h"
#include "playlist/PlaylistItem.h"

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
     * Returns the unique playlist item id of the active track
     * (or 0 if no track is active).
     * @return The playlist item's id.
     */
    virtual quint64 activeId() const = 0;

    /**
     * Returns the currently active row, translated to proxy rows
     * (or -1 if the current row is not represented by this proxy).
     * @return The currently active (playing) row in proxy terms.
     */
    virtual int activeRow() const = 0;

    /**
     * Returns a pointer to the currently active track (or a default constructed value if
     * no track is active).
     * @return A pointer to the track.
     */
    virtual Meta::TrackPtr activeTrack() const = 0;

    /**
     * Clears the current search term.
     */
    virtual void clearSearchTerm() {}    //dummy, needed by Playlist::Model

   /**
     * Returns the number of columns exposed by the current model.
     * @param parent the parent of the columns to count.
     * @return the number of columns.
     */
    virtual int columnCount( const QModelIndex& ) const = 0;

    /**
     * Reports if the current model exposes a given id.
     * @param id the id to check for.
     * @return true if the id is present, otherwise false.
     */
    virtual bool containsId( const quint64 id ) const = 0;

    /**
     * Reports if the current model exposes a given track.
     * @param track the track to check for.
     * @return true if the track is present, otherwise false.
     */
    virtual bool containsTrack( const Meta::TrackPtr track ) const = 0;

    /**
     * Get the current search fields bitmask.
     * @return The current search fields.
     */
    virtual int currentSearchFields() { return -1; } //dummy, needed by Playlist::Model

    /**
     * Get the current search term.
     * @return The curent search term.
     */
    virtual QString currentSearchTerm() { return QString(); }   //dummy, needed by Playlist::Model

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
     * Returns the item flags for the given index.
     * @param index the index to retrieve the flags for.
     * @return the item flags.
     */
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const = 0;

    /**
     * Returns the unique 64-bit id for the given row in the current model.
     * @param row the row.
     * @return the unique id.
     */
    virtual quint64 idAt( const int row ) const = 0;

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
     * Returns the row in the current model for a given unique 64-bit id.
     * @param id the id.
     * @return the row, -1 if the id is invalid.
     */
    virtual int rowForId( const quint64 id ) const = 0;

    /**
     * Sets the currently active (playing) row, translated for this proxy.
     * @param row the row to be set as active.
     */
    virtual void setActiveRow( int row ) = 0;

    /**
     * Get the state of a track by its id.
     * @param id The id of the track.
     * @return The state of the track.
     */
    virtual Item::State stateOfId( quint64 id ) const = 0;

    /**
     * Get the sate of the track at given row in the proxy model.
     * @param row The row in proxy terms.
     * @return The state of the track at the row.
     */
    virtual Item::State stateOfRow( int row ) const = 0;

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
     * Returns a pointer to the track with the given unique id.
     * @param id the id to return the track pointer for.
     * @return a pointer to the track with the given id.
     */
    virtual Meta::TrackPtr trackForId( const quint64 id ) const = 0;

    /**
     * Destructor.
     */
    virtual ~AbstractModel() {};
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTABSTRACTMODEL_H
