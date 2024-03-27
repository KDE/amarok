/****************************************************************************************
 * Copyright (c) 2007-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#ifndef PLAYLISTBROWSERNSPODCASTMODEL_H
#define PLAYLISTBROWSERNSPODCASTMODEL_H

#include "amarok_export.h"

#include "core/podcasts/PodcastMeta.h"
#include "core/podcasts/PodcastProvider.h"

#include "PlaylistBrowserModel.h"
#include "playlist/PlaylistModelStack.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QVariant>

namespace PlaylistBrowserNS {

/* TODO: these should be replaced with custom roles for PlaylistColumn so all data of a playlist can
   be fetched at once with itemData() */
enum
{
    SubtitleColumn = PlaylistBrowserModel::CustomColumOffset,
    AuthorColumn,
    KeywordsColumn,
    FilesizeColumn, // episode only
    ImageColumn,    // channel only (for now)
    DateColumn,
    IsEpisodeColumn,
    ColumnCount
};

/**
 * @author Bart Cerneels
 */
class AMAROK_EXPORT PodcastModel : public PlaylistBrowserModel
{
    Q_OBJECT

    public:
        static PodcastModel *instance();
        static void destroy();

        /* QAbstractItemModel methods */
        QVariant data(const QModelIndex &index, int role) const override;
        bool setData( const QModelIndex &index, const QVariant &value,
                              int role = Qt::EditRole ) override;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;

        Qt::DropActions supportedDropActions() const override {
            return Qt::CopyAction | Qt::MoveAction;
        }

        Qt::DropActions supportedDragActions() const override {
            return Qt::MoveAction | Qt::CopyAction;
        }

    Q_SIGNALS:
        void episodeMarkedAsNew( Podcasts::PodcastEpisodePtr );

    public Q_SLOTS:
        void addPodcast();
        void refreshPodcasts();

    private:
        static PodcastModel *s_instance;
        static QPixmap *m_shadedStar;
        PodcastModel();

        QVariant channelData( const Podcasts::PodcastChannelPtr &channel,
                              const QModelIndex &idx, int role ) const;
        QVariant episodeData( const Podcasts::PodcastEpisodePtr &episode,
                              const QModelIndex &idx, int role ) const;

        Podcasts::PodcastChannelPtr channelForIndex( const QModelIndex &index ) const;
        Podcasts::PodcastEpisodePtr episodeForIndex( const QModelIndex &index ) const;

        Q_DISABLE_COPY( PodcastModel )

        /**
         * A convenience function to convert a PodcastEpisodeList into a TrackList.
         */
        static Meta::TrackList podcastEpisodesToTracks( Podcasts::PodcastEpisodeList episodes );

        bool isOnDisk( Podcasts::PodcastEpisodePtr episode ) const;
        QVariant icon( const Podcasts::PodcastChannelPtr &channel ) const;
        QVariant icon( const Podcasts::PodcastEpisodePtr &episode ) const;
};

}

namespace The {
    AMAROK_EXPORT PlaylistBrowserNS::PodcastModel* podcastModel();
}

#endif
