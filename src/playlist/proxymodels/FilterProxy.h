/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef AMAROK_PLAYLISTFILTERPROXY_H
#define AMAROK_PLAYLISTFILTERPROXY_H

#include "ProxyBase.h"
#include "playlist/PlaylistModel.h"

namespace Playlist
{
/**
 * A proxy model used by navigators to only operate on tracks that match the current
 * paylist search term.
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class FilterProxy : public ProxyBase
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    FilterProxy( AbstractModel *belowModel, QObject *parent = 0 );

    /**
     * Destructor.
     */
    ~FilterProxy();

    int rowCount( const QModelIndex &parent = QModelIndex() ) const;

    /**
     * Find the first track in the playlist that matches the search term in one of the
     * specified search fields. This function emits found() or notFound() depending on
     * whether a match is found.
     * @param searchTerm The term to search for.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match, -1 if no match is found.
     */
    int find( const QString & searchTerm, int searchFields = MatchTrack );

    /**
     * Notify proxy that the search term of searched fields has changed. Since
     * this calls does not use the parents filter values, this method needs to be
     * called when the values change.
     */
    void filterUpdated();

    /**
     * Toggle acting in pass through mode. When in pass through mode, this proxy
     * is basically completely transparent, and ignores any search terms. It also
     * ignores any calls to filterUpdated() while in pass through mode,.
     * @param passThrough Determines whether pass through mode is enabled.
     */
    void showOnlyMatches( bool onlyMatches );

    void clearSearchTerm();

protected:
    /**
     * Reimplemented from QSortFilterProxyModel. Used internally by the proxy to
     * determine if a given row in the source model should be included in this
     * proxy. When in pass through mode, this always returns true.
     * @param source_row The row in the source model to check.
     * @param source_parent Ignored.
     * @return True if the row should be included, false otherwise.
     */
    virtual bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const;

    /**
     * Check if a given row matches the current search term with the current
     * search fields.
     * @param source_row The row to check.
     * @return True if the row matches, false otherwise.
     */
    bool matchesCurrentSearchTerm( int source_row ) const;

    /**
     * Converts a row index that's valid in the proxy below this one to a row index valid
     * in this proxy.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in this proxy.
     */
    virtual int rowFromSource( int row ) const;

    /**
     * Converts a row index that's valid in this proxy to a row index valid in the proxy
     * below this one.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in the proxy below this one.
     */
    virtual int rowToSource( int row ) const;

protected slots:
    /**
     * Slot called when the source model has inserted new tracks. Uses filterAcceptsRow
     * to determine if a given id should be included in the list forwarded to any
     * listeners in the insertedIds() signal.
     * @param ids the list of id's added to the source model.
     */
    void slotInsertedIds( const QList<quint64> &ids );

    /**
     * Slot called when the source model has removed tracks. Uses filterAcceptsRow
     * to determine if a given id should be included in the list forwarded to any
     * listeners in the removedIds() signal.
     * @param ids the list of id's removed from the source model.
     */
    void slotRemovedIds( const QList<quint64> &ids );

signals:
    /**
     * Signal forwarded from the source model.
     * @param the list of id's added that are also represented by this proxy.
     */
    void insertedIds( const QList<quint64>& );

    /**
     * Signal forwarded from the source model.
     * @param the list of id's removed that are also represented by this proxy.
     */
    void removedIds( const QList<quint64>& );

private:
    QString m_currentSearchTerm;
    int m_currentSearchFields;

    bool m_passThrough;
};

}

#endif
