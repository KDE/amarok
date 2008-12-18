/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef PLAYLISTNAVIGATORFILTERPROXYMODEL_H
#define PLAYLISTNAVIGATORFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "playlist/PlaylistItem.h"

namespace Playlist {

/**
A proxy model used by navigators that wants to only operate on tracks that match the current paylist search term

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class NavigatorFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:

    static NavigatorFilterProxyModel* instance();

    int activeRow() const;
    quint64 idAt( const int row ) const;
    Item::State stateOfRow( int row ) const;
    Item::State stateOfId( quint64 id ) const;


    /**
     * Get the first row below the currently active track that matches the
     * current search term
     * @return The first matching row.
     */
    int firstMatchAfterActive();

    /**
     * Get the first row above the currently active track that matches the
     * current search term
     * @return The first matching row.
     */
    int firstMatchBeforeActive();

    void filterUpdated();

protected:
    virtual bool filterAcceptsRow ( int row, const QModelIndex & source_parent ) const;

protected slots:

    void slotInsertedIds( const QList<quint64> &ids );
    void slotRemovedIds( const QList<quint64> &ids );

signals:
    void insertedIds( const QList<quint64>& );
    void removedIds( const QList<quint64>& );

    void filterChanged();

private:
    NavigatorFilterProxyModel();
    ~NavigatorFilterProxyModel();
    
    static NavigatorFilterProxyModel* s_instance;      //! instance variable
};

}

#endif
