/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                              *
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
#include "meta/Meta.h"

#include <QHash>
#include <QModelIndex>

namespace Playlist
{

// Extension of Playlist::DataRoles
enum GroupDataRoles
{
    GroupRole = 256,
    GroupedTracksRole, // deprecated
};

enum GroupMode
{
    None = 1,
    Head,
    Head_Collapsed, // deprecated
    Body,
    Tail,
    Collapsed // deprecated
};

class GroupingProxy : public ProxyBase
{
    Q_OBJECT

public:
    explicit GroupingProxy( AbstractModel *belowModel, QObject *parent = 0 );
    ~GroupingProxy();

    static GroupingProxy* instance();
    static void destroy();

    // functions from QAbstractProxyModel
    QVariant data( const QModelIndex &index, int role ) const;

    // grouping-related functions
    int firstInGroup( int ) const;
    int lastInGroup( int ) const;

    int tracksInGroup( int row ) const;
    int lengthOfGroup( int row ) const;

    /**
     * @param groupingCategory A string from 'groupableCategories', or "None", or empty string.
     */
    QString groupingCategory() const;
    void setGroupingCategory( const QString &groupingCategory );

//signals:
    // Emits signals inherited from QSortFilterProxy

    // Emits signals inherited from Playlist::AbstractModel / ProxyBase

private slots:
    /**
    * Handlers for the standard QAbstractItemModel signals.
    */
    void sourceDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight );
    void sourceLayoutChanged();
    void sourceModelReset();
    void sourceRowsInserted( const QModelIndex& parent, int start, int end );
    void sourceRowsRemoved( const QModelIndex& parent, int start, int end );

    void regroupAll();

private:
    void regroupRows( int firstRow, int lastRow );

    int groupRowCount( int row ) const;

    /**
     * This function is used to determine if 2 tracks belong in the same group.
     * The track pointers are allowed to be invalid.
     * @return true if track should be grouped together, false otherwise
     */
    bool shouldBeGrouped( Meta::TrackPtr track1, Meta::TrackPtr track2 );

    // Variables
    QString m_groupingCategory;
    QList<GroupMode> m_rowGroupMode;
};

} // namespace Playlist

#endif
