/***************************************************************************
 *   Copyright © 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>       *
 *             © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
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

#ifndef AMAROK_PLAYLISTSORTPROXY_H
#define AMAROK_PLAYLISTSORTPROXY_H

#include "FilterProxy.h"
#include "SortScheme.h"

#include <QSortFilterProxyModel>

namespace Playlist
{

/**
 * A ProxyModel that implements multilevel sorting for the Playlist.
 * This proxy should sit above the FilterProxy and below the GroupingProxy.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    /**
     * Returns the single instance of SortProxy.
     * @return the class instance.
     */
    static SortProxy *instance();

    /**
     * Returns the currently active row, translated to proxy rows
     * (or -1 if the current row is not represented by this proxy).
     * @return The currently active (playing) row in proxy terms.
     */
    int activeRow() const;

    /**
     * Clears the current search term.
     */
    void clearSearchTerm();

    /**
     * Forwards the number of columns from the FilterProxy as SortProxy by definition shouldn't
     * change the column count.
     * @param parent the parent of the columns to count.
     * @return the number of columns.
     */
    int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
     * Get the current search fields bitmask.
     * @return The current search fields.
     */
    int currentSearchFields();

    /**
     * Forwards the request down the proxy stack and gets the data at an index.
     * @param index the index for which to retrieve the data from the model.
     * @return the data from the model.
     */
    QVariant data( const QModelIndex& index, int role ) const;

    /**
     * Get the current search term.
     * @return The curent search term.
     */
    QString currentSearchTerm();

    /**
     * Forwards a search down through the stack of ProxyModels.
     * Find the first track in the playlist that matches the search term in one of the
     * specified search fields. Playlist::Model::find() emits found() or notFound() depending on
     * whether a match is found.
     * @param searchTerm The term to search for.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match, -1 if no match is found.
     */
    int find( const QString &searchTerm, int searchFields );

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
    int findNext( const QString &searchTerm, int selectedRow, int searchFields );

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
    int findPrevious( const QString &searchTerm, int selectedRow, int searchFields );

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

    //TODO: document me
    //these methods just forward stuff straight to the model/proxy under them
    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags( const QModelIndex& ) const;
    QStringList mimeTypes() const;
    QMimeData* mimeData( const QModelIndexList& ) const;
    bool dropMimeData( const QMimeData*, Qt::DropAction, int, int, const QModelIndex& );

    bool lessThan( const QModelIndex & left, const QModelIndex & right ) const;

public slots:
    void updateSortMap( SortScheme *scheme );

signals:
    /**
     * Signal forwarded from the source model. IDs are unique so they shouldn't be modified
     * by this proxy.
     * @param the list of id's added that are also represented by this proxy.
     */
    void insertedIds( const QList<quint64>& );

    /**
     * Signal forwarded from the source model. IDs are unique so they shouldn't be modified
     * by this proxy.
     * @param the list of id's removed that are also represented by this proxy.
     */
    void removedIds( const QList<quint64>& );

    /**
     * Signal forwarded from the FilterProxy, emitted when the proxy changes its filtering.
     */
    void filterChanged();
    
private:
    /**
     * Constructor.
     */
    SortProxy();

    /**
     * Destructor.
     */
    ~SortProxy();

    FilterProxy *m_belowModel;          //! The Proxy or Model that's right below this one in the stack of Models/Proxies.

    static SortProxy *s_instance;       //! Instance member.

    SortScheme *m_scheme;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTPROXY_H
