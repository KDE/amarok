/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009-2010 Leo Franchi <lfranchi@kde.org>                               *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef DYNAMICMODEL_H
#define DYNAMICMODEL_H

#include "Bias.h"
#include "DynamicPlaylist.h"

#include "amarok_export.h" // we are exporting it for the tests

#include <QAbstractItemModel>
#include <QList>
#include <QString>

class TestDynamicModel;

namespace Dynamic {

class BiasedPlaylist;
class AbstractBias;

class AMAROK_EXPORT DynamicModel : public QAbstractItemModel
{
    Q_OBJECT
    public:
        // role used for the model
        enum Roles
        {
            // WidgetRole = 0xf00d,
            PlaylistRole = 0xf00e,
            BiasRole = 0xf00f,

            BiasPercentage = 0xf010, // for sub-biases underneath a part bias
            BiasOperation = 0xf011   // for sub-biases underneath a and-bias
        };

        static DynamicModel* instance();

        ~DynamicModel();

        // void changePlaylist( int i );

        /** Returns the currently active playlist.
            Don't free this pointer
        */
        Dynamic::DynamicPlaylist* activePlaylist() const;
        int activePlaylistIndex() const;

        /** Find the playlist with name, make it active and return it */
        Dynamic::DynamicPlaylist* setActivePlaylist( int );

        int playlistIndex( Dynamic::DynamicPlaylist* playlist ) const;

        /** Inserts a playlist at the given index.
            If the playlist is already in the model it will be moved
            to the position
        */
        QModelIndex insertPlaylist( int index, Dynamic::DynamicPlaylist* playlist );

        /** Inserts a bias at the given index.
            The bias must not be part of a model. When in doubt call bias->replace(BiasPtr())
            to remove the bias from it's current position.
        */
        QModelIndex insertBias( int row, const QModelIndex &parentIndex, Dynamic::BiasPtr bias );

        Qt::DropActions supportedDropActions() const override;

        // --- QAbstractItemModel functions ---
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
        bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
        Qt::ItemFlags flags( const QModelIndex& index ) const override;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
        int columnCount( const QModelIndex& parent = QModelIndex() ) const override;

        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList &indexes) const override;
        bool dropMimeData(const QMimeData *data,
                          Qt::DropAction action,
                          int row, int column, const QModelIndex &parent) override;

        // ---

        /** Returns the index for the bias
            @return Returns an invalid index if the bias is not in the model.
        */
        QModelIndex index( Dynamic::BiasPtr bias ) const;

        /** Returns the index for the playlist
            @return Returns an invalid index if the playlist is not in the model.
        */
        QModelIndex index( Dynamic::DynamicPlaylist* playlist ) const;

        /** Returns a representation of the whole model for debugging */
        QString toString();

    Q_SIGNALS:
        void activeChanged( int index ); // active row changed

    public Q_SLOTS:
        /** Saves all playlists to disk */
        void savePlaylists();

        /** Loads the last saved playlists form disk */
        void loadPlaylists();

        /** Removes the playlist or bias at the given index. */
        void removeAt( const QModelIndex& index );

        /** Clone the playlist or bias at the given index. */
        QModelIndex cloneAt( const QModelIndex& index );

        /** Creates a new playlist and returns the index to it. */
        QModelIndex newPlaylist();

    private:
        // two functions to search for parents
        QModelIndex parent( int row, Dynamic::BiasedPlaylist* list, Dynamic::BiasPtr bias ) const;
        QModelIndex parent( int row, Dynamic::BiasPtr parent, Dynamic::BiasPtr bias ) const;

        /** Writes the index to the data stream */
        void serializeIndex( QDataStream *stream, const QModelIndex& index ) const;

        /** Gets an index from the data stream */
        QModelIndex unserializeIndex( QDataStream *stream ) const;

        Dynamic::BiasedPlaylist* cloneList( Dynamic::BiasedPlaylist* list );
        Dynamic::BiasPtr cloneBias( Dynamic::BiasPtr bias );

        // -- model change signals ---
        // The following functions are called by the biases to
        // notify the model about changes.

        void playlistChanged( Dynamic::DynamicPlaylist* playlist );
        void biasChanged( Dynamic::BiasPtr bias );

        void beginRemoveBias( Dynamic::BiasedPlaylist* parent );
        void beginRemoveBias( Dynamic::BiasPtr parent, int index );
        void endRemoveBias();

        void beginInsertBias( Dynamic::BiasedPlaylist* parent );
        void beginInsertBias( Dynamic::BiasPtr parent, int index );
        void endInsertBias();

        void beginMoveBias( Dynamic::BiasPtr parent, int from, int to );
        void endMoveBias();

        // ----

        bool savePlaylists( const QString &filename );
        bool loadPlaylists( const QString &filename );
        void initPlaylists();

        DynamicModel(QObject* parent = nullptr);
        static DynamicModel* s_instance;

        int m_activePlaylistIndex;

        /** Contains all the dynamic playlists.  */
        QList<Dynamic::DynamicPlaylist*> m_playlists;

        friend class Dynamic::DynamicPlaylist;
        friend class Dynamic::BiasedPlaylist;
        friend class Dynamic::AndBias;

        friend class ::TestDynamicModel;
};

}

#endif

