/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLISTGROUPINGPROXY_H
#define AMAROK_PLAYLISTGROUPINGPROXY_H

#include "ProxyBase.h"
#include "core/meta/Meta.h"

#include <QHash>
#include <QModelIndex>

namespace Playlist
{

// Extension of Playlist::DataRoles
enum GroupDataRoles
{
    GroupRole = 256,
    GroupedTracksRole // deprecated
};

namespace Grouping
{
enum GroupMode
{
    None = 1,
    Head,
    Head_Collapsed, // deprecated
    Body,
    Tail,
    Collapsed, // deprecated
    Invalid
};
}

class GroupingProxy : public ProxyBase
{
    Q_OBJECT

public:
    explicit GroupingProxy( AbstractModel *belowModel, QObject *parent = 0 );
    ~GroupingProxy();

    static GroupingProxy* instance();
    static void destroy();


    //! Configuration
    /**
     * The criterium by which adjacent items are divided into groups.
     * @param groupingCategory A string from 'groupableCategories', or "None", or empty string.
     */
    QString groupingCategory() const;
    void setGroupingCategory( const QString &groupingCategory );


    //! Grouping info functions
    /**
     * @return true if 'index' is the first item of a Group.
     */
    bool isFirstInGroup( const QModelIndex & index );

    /**
     * @return true if 'index' is the last item of a Group.
     */
    bool isLastInGroup( const QModelIndex & index );

    /**
     * @return The first item of the Group that 'index' belongs to.
     */
    QModelIndex firstIndexInSameGroup( const QModelIndex & index );

    /**
     * @return The last item of the Group that 'index' belongs to.
     */
    QModelIndex lastIndexInSameGroup( const QModelIndex & index );

    /**
     * @return The number of items in the Group that 'index' belongs to.
     */
    int groupRowCount( const QModelIndex & index );

    /**
     * @return The play length (in seconds) of the Group that 'index' belongs to.
     */
    int groupPlayLength( const QModelIndex & index );


    //! Custom version of functions inherited from QSortFilterProxyModel
    QVariant data( const QModelIndex &index, int role ) const;

//signals:
    // Emits signals inherited from QSortFilterProxy

    // Emits signals inherited from Playlist::AbstractModel / ProxyBase

private slots:
    /**
    * Handlers for the standard QAbstractItemModel signals.
    */
    void proxyDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight );
    void proxyLayoutChanged();
    void proxyModelReset();
    void proxyRowsInserted( const QModelIndex& parent, int start, int end );
    void proxyRowsRemoved( const QModelIndex& parent, int start, int end );

private:
    /**
     * This function determines the "Status within the group" of a model row.
     */
    Grouping::GroupMode groupModeForIndex( const QModelIndex & index );

    /**
     * This function is used to determine if 2 tracks belong in the same group.
     * The track pointers are allowed to be invalid.
     * @return true if track should be grouped together, false otherwise
     */
    bool shouldBeGrouped( Meta::TrackPtr track1, Meta::TrackPtr track2 );

    /**
     * Invalidate any cached assumptions about model rows.
     */
    void invalidateGrouping();


    // Variables
    QString m_groupingCategory;
    int m_groupingCategoryIndex;

    QHash<int, Grouping::GroupMode> m_cachedGroupModeForRow;
};

} // namespace Playlist

#endif
