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

#ifndef AMAROK_METAPLAYLISTMODEL_H
#define AMAROK_METAPLAYLISTMODEL_H

#include "core/playlists/Playlist.h"
#include "playlist/PlaylistModelStack.h"

#include <QAbstractItemModel>

//Playlist & Track index differentiator macros
//QModelIndex::intenalId() is a qint64 to support 64-bit pointers in a union with the ID
#define TRACK_MASK (0x1<<31)
#define IS_TRACK(x) ((x.internalId()) & (TRACK_MASK))?true:false
#define SET_TRACK_MASK(x) ((x) | (TRACK_MASK))
#define REMOVE_TRACK_MASK(x) ((x) & ~(TRACK_MASK))

class QAction;

typedef QList<QAction *> QActionList;

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QActionList )

namespace PlaylistBrowserNS {

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class MetaPlaylistModel : public QAbstractItemModel,
                          public Meta::PlaylistObserver
{
    Q_OBJECT
    public:
        enum {
            PlaylistColumn = 0, //Data form the playlist itself
            LabelColumn, //Data from the labels. Can be used as foldernames in the view.
            ProviderColumn, //data from the PlaylistProvider
            CustomColumOffset //first column that can be used by subclasses for their own data
        };

        enum
        {
            DescriptionRole = Qt::UserRole,
            ByLineRole, //show some additional info like count or status. Displayed under description
            ActionCountRole,
            ActionRole, //list of QActions for the index
            CustomRoleOffset //first role that can be used by sublasses for their own data
        };

        MetaPlaylistModel( int PlaylistCategory );
        virtual ~MetaPlaylistModel() {}

        /* QAbstractItemModel methods */
        virtual QVariant data( const QModelIndex &index, int role ) const;
        virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
        virtual QVariant headerData( int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole ) const;
        virtual QModelIndex index( int row, int column,
                        const QModelIndex &parent = QModelIndex() ) const;
        virtual QModelIndex parent( const QModelIndex &index ) const;
        virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
        virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;

        virtual QStringList mimeTypes() const;
        QMimeData* mimeData( const QModelIndexList &indexes ) const;

        /* Meta::PlaylistObserver methods */
        virtual void trackAdded( Meta::PlaylistPtr playlist, Meta::TrackPtr track, int position );
        virtual void trackRemoved( Meta::PlaylistPtr playlist, int position );

    public slots:
        void slotRenamePlaylist( Meta::PlaylistPtr playlist );
        void slotUpdate();

    signals:
        void renameIndex( const QModelIndex &index );

    protected:
        Meta::PlaylistList loadPlaylists();

        Meta::TrackList tracksFromIndexes( const QModelIndexList &list ) const;
        Meta::TrackPtr trackFromIndex( const QModelIndex &index ) const;
        Meta::PlaylistPtr playlistFromIndex( const QModelIndex &index ) const;

        Meta::PlaylistList m_playlists;
        QActionList actionsFor( const QModelIndex &idx ) const;

        PlaylistProvider *getProviderByName( const QString &name );

    private slots:
        void slotLoad();
        void slotAppend();

    private:
        int m_playlistCategory;
        QAction *m_appendAction;
        QAction *m_loadAction;
};

}

//we store these in a QVariant for the load and append actions
Q_DECLARE_METATYPE( QModelIndexList )

#endif //AMAROK_METAPLAYLISTMODEL_H
