/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROK_PLAYLISTMODEL_H
#define AMAROK_PLAYLISTMODEL_H

#include "UndoCommands.h"
#include "amarok_export.h"
#include "core/meta/Observer.h"
#include "core/support/Amarok.h"
#include "playlist/proxymodels/AbstractModel.h"

#include <QAbstractListModel>
#include <QHash>
#include <QTimer>

#include <QtGlobal>

class QMimeData;
class QModelIndex;
class TestPlaylistModels;

namespace Playlist
{

class AMAROK_EXPORT Model : public QAbstractListModel, public Meta::Observer, public Playlist::AbstractModel
{
    friend class InsertTracksCmd;
    friend class RemoveTracksCmd;
    friend class MoveTracksCmd;
    friend class ::TestPlaylistModels; //this test really needs access to the private functions.

    Q_OBJECT

    public:
        explicit Model( QObject *parent = nullptr );
        ~Model();

        // Inherited from QAbstractItemModel  (via QAbstractListModel)
        int columnCount( const QModelIndex& parent = QModelIndex() ) const override { Q_UNUSED( parent ); return NUM_COLUMNS; }
        QVariant data( const QModelIndex& index, int role ) const override;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent ) override;
        Qt::ItemFlags flags( const QModelIndex &index ) const override;
        QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;
        QMimeData* mimeData( const QModelIndexList &indexes ) const override;
        QStringList mimeTypes() const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override { Q_UNUSED( parent ); return m_items.size(); }
        Qt::DropActions supportedDropActions() const override;

        // Inherited from Playlist::AbstractModel
        QAbstractItemModel* qaim() const override { return const_cast<Model*>( this ); }

        quint64 activeId() const override;
        Meta::TrackPtr activeTrack() const override;
        int activeRow() const override { return m_activeRow; } // returns -1 if there is no active row

        bool containsTrack( const Meta::TrackPtr& track ) const override;
        int firstRowForTrack( const Meta::TrackPtr& track ) const override;
        QSet<int> allRowsForTrack( const Meta::TrackPtr& track ) const override;

        quint64 idAt( const int row ) const override;
        bool rowExists( int row ) const override { return (( row >= 0 ) && ( row < m_items.size() ) ); }
        int rowForId( const quint64 id ) const override; // returns -1 if the id is invalid
        int rowFromBottomModel( const int row ) override { return row; }
        int rowToBottomModel( const int row ) override { return row; }
        void setActiveId( const quint64 id ) override { setActiveRow( rowForId( id ) ); }
        void setActiveRow( int row ) override;
        void setAllNewlyAddedToUnplayed();
        void setAllUnplayed() override;
        void emitQueueChanged() override;
        int queuePositionOfRow( int row ) override;
        Item::State stateOfId( quint64 id ) const override;
        Item::State stateOfRow( int row ) const override;
        qint64 totalLength() const override { return m_totalLength; }
        quint64 totalSize() const override { return m_totalSize; }
        Meta::TrackPtr trackAt( int row ) const override;
        Meta::TrackPtr trackForId( const quint64 id ) const override;

        bool exportPlaylist( const QString &path, bool relative = false ) override;
        Meta::TrackList tracks() override;

        // Inherited from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( const Meta::TrackPtr &track ) override;
        void metadataChanged( const Meta::AlbumPtr &album ) override;

        // static member functions
        static QString prettyColumnName( Column index ); //!< takes a Column enum and returns its string name

        /** Set the columns that are displayed in the tooltip */
        static void setTooltipColumns( bool columns[] );

        static void enableToolTip( bool enable );

    Q_SIGNALS:
        void activeTrackChanged( quint64 );
        void queueChanged();

    protected:
        int rowForItem( Item *item ) const { return m_items.indexOf( item ); }

    private Q_SLOTS:
        void saveState();
        void queueSaveState();
        void insertTracksFromTrackLoader( const Meta::TrackList &tracks );

    private:
        QString tooltipFor( Meta::TrackPtr track ) const;

        // Inherited from QAbstractItemModel. Make them private so that nobody is tempted to use them.
        bool insertRow( int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool insertRows( int, int, const QModelIndex& parent = QModelIndex() ) override { Q_UNUSED( parent ); return false; }
        bool removeRow( int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool removeRows( int, int, const QModelIndex& parent = QModelIndex() ) override { Q_UNUSED( parent ); return false; }

        // These functions do the real work of modifying the playlist, and should be called ONLY by UndoCommands
        void insertTracksCommand( const InsertCmdList& );
        void removeTracksCommand( const RemoveCmdList& );
        void moveTracksCommand( const MoveCmdList&, bool reverse = false );
        void clearCommand();

        // Always alter the state of a row via one of the following functions.
        void setStateOfItem_batchStart();
        void setStateOfItem_batchEnd();
        void setStateOfItem( Item *item, int row, Item::State state );    // 'item' must equal 'm_items.at( row )'
        void setStateOfItem( Item *item, Item::State state ) { setStateOfItem( item, rowForItem( item ), state ); }
        void setStateOfRow( int row, Item::State state )     { setStateOfItem( m_items.at( row ), row, state ); }

        // Variables
        QList<Item*> m_items;               //!< list of playlist items, in their "natural" (unsorted) order.
        QHash<quint64, Item*> m_itemIds;    //!< maps playlist item ID to Item pointer.

        int m_activeRow;    //!< the row being played

        qint64 m_totalLength;
        quint64 m_totalSize;

        QString m_playlistName;
        bool m_proposeOverwriting;

        int m_setStateOfItem_batchMinRow;    //!< For 'setStateOfItem_batch*()'
        int m_setStateOfItem_batchMaxRow;

        static bool s_tooltipColumns[NUM_COLUMNS];
        static bool s_showToolTip;

        QTimer *m_saveStateTimer;
};

} // namespace Playlist

#endif
