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
enum GroupDataRoles
{
    GroupRole = 256,
    GroupedTracksRole, // deprecated
    GroupedAlternateRole // deprecated
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
    void setCollapsed( int, bool ) const;
    int firstInGroup( int ) const;
    int lastInGroup( int ) const;

    int tracksInGroup( int row ) const;
    int lengthOfGroup( int row ) const;

    QString groupingCategory() const;
    void setGroupingCategory( const QString &groupingCategory );

signals:
    /**
     * This signal is emitted when tracks are added to the playlist.
     * @param parent the parent index.
     * @param start the row number of the first track that has been added.
     * @param end the row number of the last track that has been added.
     */
    void rowsInserted( const QModelIndex& parent, int start, int end );

    /**
     * This signal is emitted when tracks are removed from the playlist.
     * @param parent the parent index.
     * @param start the row number of the first track that has been removed.
     * @param end the row number of the last track that has been removed.
     */
    void rowsRemoved( const QModelIndex& parent, int start, int end );

    /**
     * This signal is emitted when tracks are (de)queued from the playlist.
     */
    void queueChanged();

private slots:
    void modelDataChanged( const QModelIndex&, const QModelIndex& );
    void modelRowsInserted( const QModelIndex&, int, int );
    void modelRowsRemoved( const QModelIndex&, int, int );
    void regroupAll();

private:
    void regroupRows( int firstRow, int lastRow );
    QList<GroupMode> m_rowGroupMode;

    // grouping auxiliary functions -- deprecated, but used by GraphicsView
    int groupRowCount( int row ) const;

    QString m_groupingCategory;

    /**
     * This function is used to determine if 2 tracks belong in the same group.
     * The current implementation is a bit of a hack, but is what gives the best
     * user experience.
     * @param track1 The first track
     * @param track2 The second track
     * @return true if track should be grouped together, false otherwise
     */
    bool shouldBeGrouped( Meta::TrackPtr track1, Meta::TrackPtr track2 );
};
} // namespace Playlist

#endif
