/****************************************************************************************
 * Copyright (c) 2009-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#ifndef AMAROK_PLAYLISTBROWSERMODEL_H
#define AMAROK_PLAYLISTBROWSERMODEL_H

#include "core/playlists/Playlist.h"
#include "core/playlists/PlaylistProvider.h"

#include <QAbstractItemModel>
#include <QAction>
//Playlist & Track index differentiator macros
//QModelIndex::intenalId() is a qint64 to support 64-bit pointers in a union with the ID
#define TRACK_MASK (0x1<<31)
#define IS_TRACK(x) ((x.internalId()) & (TRACK_MASK))?true:false
#define SET_TRACK_MASK(x) ((x) | (TRACK_MASK))
#define REMOVE_TRACK_MASK(x) ((x) & ~(TRACK_MASK))

class QAction;

Q_DECLARE_METATYPE( QActionList )

namespace PlaylistBrowserNS {

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class PlaylistBrowserModel : public QAbstractItemModel, public Playlists::PlaylistObserver
{
    Q_OBJECT
    public:
        enum {
            PlaylistItemColumn = 0, //Data form the playlist itself or it's tracks
            LabelColumn, //Data from the labels. Can be used as foldernames in the view.
            ProviderColumn, //data from the PlaylistProvider
            CustomColumOffset //first column that can be used by subclasses for their own data
        };

        enum
        {
            ProviderRole = Qt::UserRole + 21, // pointer to associated PlaylistProvider
            PlaylistRole = Qt::UserRole + 22, // PlaylistPtr for associated playlist or null
            TrackRole = Qt::UserRole + 23, // TrackPtr for associated track or null
            EpisodeIsNewRole = Qt::UserRole + 24, // for podcast episodes, supports setting, type: bool
            CustomRoleOffset = Qt::UserRole + 25 //first role that can be used by subclasses for their own data
        };

        PlaylistBrowserModel( int PlaylistCategory );
        ~PlaylistBrowserModel() override {}

        /* QAbstractItemModel methods */
        QVariant data( const QModelIndex &index, int role ) const override;
        bool setData( const QModelIndex &idx, const QVariant &value, int role ) override;
        Qt::ItemFlags flags( const QModelIndex &index ) const override;
        QVariant headerData( int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole ) const override;
        QModelIndex index( int row, int column,
                        const QModelIndex &parent = QModelIndex() ) const override;
        QModelIndex parent( const QModelIndex &index ) const override;

        bool hasChildren( const QModelIndex &parent = QModelIndex() ) const override;
        int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
        int columnCount( const QModelIndex &parent = QModelIndex() ) const override;

        bool canFetchMore( const QModelIndex &parent ) const override;
        void fetchMore( const QModelIndex &parent ) override;

        QStringList mimeTypes() const override;
        QMimeData* mimeData( const QModelIndexList &indexes ) const override;
        bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row,
                                   int column, const QModelIndex &parent ) override;

        /* Playlists::PlaylistObserver methods */
        void metadataChanged( const Playlists::PlaylistPtr &playlist ) override;
        void trackAdded( const Playlists::PlaylistPtr &playlist, const Meta::TrackPtr &track, int position ) override;
        void trackRemoved( const Playlists::PlaylistPtr &playlist, int position ) override;

    public Q_SLOTS:
        void slotRenamePlaylist( Playlists::PlaylistPtr playlist );
        void slotUpdate( int category );

    Q_SIGNALS:
        void renameIndex( const QModelIndex &index );

    protected:
        virtual Playlists::PlaylistList loadPlaylists();

        Meta::TrackList tracksFromIndexes( const QModelIndexList &list ) const;
        Meta::TrackPtr trackFromIndex( const QModelIndex &index ) const;
        Playlists::PlaylistPtr playlistFromIndex( const QModelIndex &index ) const;
        Playlists::PlaylistProvider *providerForIndex( const QModelIndex &index ) const;

        Playlists::PlaylistList m_playlists;
        QMap<Playlists::PlaylistPtr,int> m_playlistTracksLoaded;

        Playlists::PlaylistProvider *getProviderByName( const QString &name );

    private Q_SLOTS:
        void slotPlaylistAdded( Playlists::PlaylistPtr playlist, int category );
        void slotPlaylistRemoved( Playlists::PlaylistPtr playlist, int category );
        void slotPlaylistUpdated( Playlists::PlaylistPtr playlist, int category );

    private:
        int m_playlistCategory;
};

}

#endif //AMAROK_PLAYLISTBROWSERMODEL_H
