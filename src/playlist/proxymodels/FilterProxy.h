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

#ifndef AMAROK_PLAYLISTFILTERPROXY_H
#define AMAROK_PLAYLISTFILTERPROXY_H

#include "ProxyBase.h"

namespace Playlist
{
/**
 * A proxy model used by navigators to only operate on tracks that match the current
 * paylist search term.
 * @author Nikolaj Hald Nielsen <nhn@kde.org>
 */
class FilterProxy : public ProxyBase
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    explicit FilterProxy( AbstractModel *belowModel, QObject *parent = 0 );

    /**
     * Destructor.
     */
    ~FilterProxy();

    /**
     * Implementation of Playlist::AbstractModel: search/filter-related functions.
     */
    void clearSearchTerm();
    int find( const QString & searchTerm, int searchFields = MatchTrack );
    void filterUpdated();
    void showOnlyMatches( bool onlyMatches );

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

//signals:
    // Emits signals inherited from QSortFilterProxy

    // Emits signals inherited from Playlist::AbstractModel / ProxyBase

private:
    QString m_currentSearchTerm;
    int m_currentSearchFields;

    bool m_showOnlyMatches;
};

} // namespace Playlist

#endif
