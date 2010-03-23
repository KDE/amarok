/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "proxymodels/AbstractModel.h"
#include "Amarok.h"
#include "UndoCommands.h"
#include "meta/Meta.h"

#include <QAbstractListModel>
#include <QHash>

#include <KLocale>
#include <kdemacros.h>

class AmarokMimeData;
class QMimeData;
class QModelIndex;


namespace Playlist
{

class AMAROK_EXPORT Model : public QAbstractListModel, public Meta::Observer, public AbstractModel
{
    friend class InsertTracksCmd;
    friend class RemoveTracksCmd;
    friend class MoveTracksCmd;

    Q_OBJECT

    public:
        Model( QObject *parent = 0 );
        ~Model();

        // inherited from QAbstractListModel
        int rowCount( const QModelIndex& parent = QModelIndex() ) const { Q_UNUSED( parent ); return m_items.size(); }
        int columnCount( const QModelIndex& parent = QModelIndex() ) const { Q_UNUSED( parent ); return NUM_COLUMNS; }
        QVariant headerData( int section, Qt::Orientation orientation, int role ) const;
        QVariant data( const QModelIndex& index, int role ) const;
        Qt::DropActions supportedDropActions() const;
        Qt::ItemFlags flags( const QModelIndex &index ) const;
        QStringList mimeTypes() const;
        QMimeData* mimeData( const QModelIndexList &indexes ) const;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

        // inherited from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( Meta::TrackPtr track );
        void metadataChanged( Meta::AlbumPtr album );

        qint64 totalLength() const { return m_totalLength; }
        quint64 totalSize() const { return m_totalSize; }

        // convenience access methods
        bool rowExists( int row ) const { return (( row >= 0 ) && ( row < m_items.size() ) ); }
        int activeRow() const { return m_activeRow; } // returns -1 if there is no active row
        void setActiveRow( int row );
        void setAllNewlyAddedToUnplayed();
        void setAllUnplayed();
        void setRowQueued( int row );
        void setRowDequeued( int row );
        Item::State stateOfRow( int row ) const;

        bool containsTrack( const Meta::TrackPtr track ) const;
        int firstRowForTrack( const Meta::TrackPtr track ) const;
        QSet<int> allRowsForTrack( const Meta::TrackPtr track ) const;

        Meta::TrackPtr trackAt( int row ) const;
        Meta::TrackPtr activeTrack() const;

        /**
         * Get the ordered list of tracks from this model
         */
        virtual Meta::TrackList tracks() const;

        // position-independent access methods
        // these are useful when you care what tracks are in the playlist, but not what order they're in (eg, the Random Track navigator)
        bool containsId( const quint64 id ) const { return m_itemIds.contains( id ); }
        int rowForId( const quint64 id ) const; // returns -1 if the id is invalid
        Meta::TrackPtr trackForId( const quint64 id ) const;

        /**
         * Returns the unique playlist id of the track at the requested row
         * @param row the index in the playlist
         * @return the id of the row specified, or 0 if the row does not exist
         */
        quint64 idAt( const int row ) const;

        /**
         * Returns the unique playlist item id of the active track
         * (or 0 if no track is active).
         * @return The playlist item's id.
         */
        quint64 activeId() const;

        /**
         * Set the active track based on the playlist id given.
         * @param id the unique playlist id
         */
        void setActiveId( const quint64 id ) { setActiveRow( rowForId( id ) ); }

        /**
         * The Item state of the playlist track at the specified index
         */
        Item::State stateOfId( quint64 id ) const;

        // methods to save playlist to file
        /**
         * Saves a playlist to a specified location.
         * @param path the path of the playlist file, as chosen by a FileDialog in MainWindow.
         */
        virtual bool exportPlaylist( const QString &path ) const;
        inline const QString defaultPlaylistPath() const { return Amarok::saveLocation() + "current.xspf"; }

        // static member functions
        static QString prettyColumnName( Column index ); //!takes a Column enum and returns its string name

        int rowToBottomModel( const int row ) { return row; }

    signals:
        void insertedIds( const QList<quint64>& );
        void activeTrackChanged( quint64 );
        void queueChanged();

    protected:
        int rowForItem( Item *item ) const { return m_items.indexOf( item ); }

    private:
        // inherit from QAbstractListModel, and make private so that nobody uses them
        bool insertRow( int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool insertRows( int, int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool removeRow( int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }
        bool removeRows( int, int, const QModelIndex& parent = QModelIndex() ) { Q_UNUSED( parent ); return false; }

        // these functions do the real work of modifying the playlist, and should be called ONLY by UndoCommands
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
        QList<Item*> m_items;               //! list of playlist items, in their "natural" (unsorted) order.
        QHash<quint64, Item*> m_itemIds;    //! maps playlist item ID to Item pointer.

        int m_activeRow;    //! the row being played

        qint64 m_totalLength;
        quint64 m_totalSize;

        QString m_playlistName;
        bool m_proposeOverwriting;

        int m_setStateOfItem_batchMinRow;    //! For 'setStateOfItem_batch*()'
        int m_setStateOfItem_batchMaxRow;
};

} // namespace Playlist

#endif
