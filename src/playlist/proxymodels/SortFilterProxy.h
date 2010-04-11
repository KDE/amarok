/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#ifndef AMAROK_PLAYLIST_SORTFILTERPROXY_H
#define AMAROK_PLAYLIST_SORTFILTERPROXY_H

#include "ProxyBase.h"

#include "SortAlgorithms.h"
#include "SortScheme.h"


namespace Playlist
{

/**
 * Sorting interface; rest of Amarok shouldn't care whether sort is in 1 QSFPM with filter or not,
 */
class SortProxy
{
    public:
        /**
         * Destructor.
         */
        virtual ~SortProxy() { }

        /**
         * Checks if the SortProxy is currently applying a SortScheme.
         * @return true if the SortProxy is sorting, otherwise false.
         */
        virtual bool isSorted() = 0;

        /**
         * Applies a sorting scheme to the playlist.
         * @param scheme the sorting scheme that will be applied.
         */
        virtual void updateSortMap( SortScheme scheme ) = 0;
};


/**
 * A proxy model that:
 *   - Does multilevel sorting on the Playlist.
 *   - Filters the Playlist based on a search term.
 */
class SortFilterProxy : public ProxyBase, public SortProxy
{
    Q_OBJECT

    public:
        //! Basics.
        explicit SortFilterProxy( AbstractModel *belowModel, QObject *parent = 0 );
        ~SortFilterProxy();

        //! Sort-related functions.
        //!   SortProxy public functions
        bool isSorted();
        void updateSortMap( SortScheme scheme );

        //! Filter-related functions.
        //!   Playlist::AbstractModel search-related functions.
        void clearSearchTerm();
        void filterUpdated();
        int find( const QString & searchTerm, int searchFields = MatchTrack );
        void showOnlyMatches( bool onlyMatches );

    //!signals:
        //! Emits signals inherited from QSortFilterProxy
        //! Emits signals inherited from Playlist::AbstractModel / ProxyBase

    private:
        /**
         * Reimplemented from QSortFilterProxyModel. The sort comparison function.
         * @param left the first index to compare.
         * @param right the second index to compare.
         * @return true if left is to be placed before right, false otherwise.
         */
        bool lessThan( const QModelIndex &left, const QModelIndex &right ) const;

        /**
         * Reimplemented from QSortFilterProxyModel. The filter decision function.
         * When not in 'showOnlyMatches' mode, this always returns true.
         * @param rowInSourceModel The row in 'sourceModel()' to check.
         * @param source_parent Ignored.
         * @return True if the row should be included, false otherwise.
         */
        bool filterAcceptsRow( int sourceModelRow, const QModelIndex &sourceModelParent ) const;


        SortScheme m_scheme;               //!< The current sorting scheme.
        multilevelLessThan m_mlt;          //!< Decision object for current sorting scheme.

        QString m_currentSearchTerm;
        int m_currentSearchFields;

        bool m_showOnlyMatches;
};

} // namespace Playlist

#endif
