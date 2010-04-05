/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef AMAROK_PROXYBASE_H
#define AMAROK_PROXYBASE_H

#include "AbstractModel.h"
#include "playlist/PlaylistItem.h"

#include <QSortFilterProxyModel>

namespace Playlist
{

/**
 * A ProxyModel that implements all the common forwarders for the interface of any
 * playlist proxy.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class ProxyBase : public QSortFilterProxyModel, public Playlist::AbstractModel
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    explicit ProxyBase( AbstractModel *belowModel, QObject *parent = 0 );

    /**
     * Destructor.
     */
    virtual ~ProxyBase();

    //! Inherited from Playlist::AbstractModel
    QAbstractItemModel* qaim() const { return const_cast<ProxyBase*>( this ); }

    virtual quint64 activeId() const;
    virtual int activeRow() const;
    virtual Meta::TrackPtr activeTrack() const;
    virtual QSet<int> allRowsForTrack( const Meta::TrackPtr track ) const;
    virtual void clearSearchTerm();
    virtual bool containsTrack( const Meta::TrackPtr track ) const;
    virtual int currentSearchFields();
    virtual QString currentSearchTerm();
    virtual bool exportPlaylist( const QString &path ) const;
    virtual void filterUpdated();
    virtual int find( const QString &searchTerm, int searchFields );
    virtual int findNext( const QString &searchTerm, int selectedRow, int searchFields );
    virtual int findPrevious( const QString &searchTerm, int selectedRow, int searchFields );
    virtual int firstRowForTrack( const Meta::TrackPtr track ) const;
    virtual quint64 idAt( const int row ) const;
    virtual bool rowExists( int row ) const;
    virtual int rowForId( const quint64 id ) const;
    virtual int rowToBottomModel( const int rowInProxy );
    virtual void setActiveId( const quint64 id );
    virtual void setActiveRow( int row );
    virtual void setAllUnplayed();
    virtual void setRowQueued( int row );
    virtual void setRowDequeued( int row );
    virtual void showOnlyMatches( bool onlyMatches );
    virtual Item::State stateOfId( quint64 id ) const;
    virtual Item::State stateOfRow( int row ) const;
    virtual qint64 totalLength() const;
    virtual quint64 totalSize() const;
    virtual Meta::TrackPtr trackAt( int row ) const;
    virtual Meta::TrackPtr trackForId( const quint64 id ) const;
    virtual Meta::TrackList tracks() const;

signals:
    //! Proxied from Playlist::Model.
    void activeTrackChanged( const quint64 );
    void queueChanged();

protected:
    /**
     * Check if a certain row in the source model matches a search term when looking at
     * the fields specified by the searchFields bitmask.
     * @param row The row number in the source model to match against.
     * @param searchTerm The search term.
     * @param searchFields A bitmask containing the fields that should be matched against.
     * @return True if a match is found in any field, false otherwise.
     */
    bool rowMatch( int sourceModelRow, const QString &searchTerm, int searchFields ) const;

    /**
     * Converts a row number in the underlying model to a row number in this proxy.
     * @param rowInSource the row number that's valid in 'sourceModel()'.
     * @return the row number that's valid in this proxy.
     */
    virtual int rowFromSource( int sourceModelRow ) const;

    /**
     * Converts a row number in this proxy to a row number in the underlying model.
     *
     * As a special case, 'rowInProxy == rowCount()' returns the bottom model's
     * 'rowCount()'. See comment at 'rowToBottomModel()'.
     *
     * @param rowInProxy the row number that's valid in this proxy.
     * @return the row number that's valid in 'sourceModel()'.
     */
    virtual int rowToSource( int proxyModelRow ) const;


    AbstractModel *m_belowModel;
};

}   //namespace Playlist

#endif  //AMAROK_PROXYBASE_H
