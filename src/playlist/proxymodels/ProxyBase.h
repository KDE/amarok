/****************************************************************************************
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
 * @author Téo Mrnjavac <teo@kde.org>
 */
class ProxyBase : public QSortFilterProxyModel, public Playlist::AbstractModel
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    explicit ProxyBase( AbstractModel *belowModel, QObject *parent = nullptr );

    /**
     * Destructor.
     */
    virtual ~ProxyBase();

    //! Inherited from Playlist::AbstractModel
    QAbstractItemModel* qaim() const override { return const_cast<ProxyBase*>( this ); }

    quint64 activeId() const override;
    int activeRow() const override;
    Meta::TrackPtr activeTrack() const override;
    QSet<int> allRowsForTrack( const Meta::TrackPtr& track ) const override;
    void clearSearchTerm() override;
    bool containsTrack( const Meta::TrackPtr& track ) const override;
    int currentSearchFields() override;
    QString currentSearchTerm() override;
    bool exportPlaylist( const QString &path, bool relative = false ) override;
    void filterUpdated() override;
    int find( const QString &searchTerm, int searchFields ) override;
    int findNext( const QString &searchTerm, int selectedRow, int searchFields ) override;
    int findPrevious( const QString &searchTerm, int selectedRow, int searchFields ) override;
    int firstRowForTrack( const Meta::TrackPtr& track ) const override;
    quint64 idAt( const int row ) const override;
    bool rowExists( int row ) const override;
    int rowForId( const quint64 id ) const override;
    int rowFromBottomModel( const int rowInBase ) override;
    int rowToBottomModel( const int rowInProxy ) override;
    void setActiveId( const quint64 id ) override;
    void setActiveRow( int row ) override;
    void setAllUnplayed() override;
    void emitQueueChanged() override;
    int queuePositionOfRow( int row ) override;
    void showOnlyMatches( bool onlyMatches ) override;
    Item::State stateOfId( quint64 id ) const override;
    Item::State stateOfRow( int row ) const override;
    qint64 totalLength() const override;
    quint64 totalSize() const override;
    Meta::TrackPtr trackAt( int row ) const override;
    Meta::TrackPtr trackForId( const quint64 id ) const override;
    Meta::TrackList tracks() override;

Q_SIGNALS:
    //! Proxied from Playlist::Model.
    void activeTrackChanged( const quint64 );
    void queueChanged();

protected:
    /**
     * Check if a certain row in the source model matches a search term when looking at
     * the fields specified by the searchFields bitmask.
     * @param sourceModelRow The row number in the source model to match against.
     * @param searchTerm The search term.
     * @param searchFields A bitmask containing the fields that should be matched against.
     * @return True if a match is found in any field, false otherwise.
     */
    bool rowMatch( int sourceModelRow, const QString &searchTerm, int searchFields ) const;

    /**
     * Converts a row number in the underlying model to a row number in this proxy.
     * @param sourceModelRow the row number that's valid in 'sourceModel()'.
     * @return the row number that's valid in this proxy.
     */
    virtual int rowFromSource( int sourceModelRow ) const;

    /**
     * Converts a row number in this proxy to a row number in the underlying model.
     *
     * As a special case, 'proxyModelRow == rowCount()' returns the bottom model's
     * 'rowCount()'. See comment at 'rowToBottomModel()'.
     *
     * @param proxyModelRow the row number that's valid in this proxy.
     * @return the row number that's valid in 'sourceModel()'.
     */
    virtual int rowToSource( int proxyModelRow ) const;


    AbstractModel *m_belowModel;
};

}   //namespace Playlist

#endif  //AMAROK_PROXYBASE_H
