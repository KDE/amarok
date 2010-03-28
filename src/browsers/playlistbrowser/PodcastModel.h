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

#include "core/podcasts/PodcastMeta.h"
#include "core/podcasts/PodcastProvider.h"

#include "MetaPlaylistModel.h"
#include "playlist/PlaylistModelStack.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QVariant>

class OpmlOutline;
class PodcastProvider;

namespace PlaylistBrowserNS {

enum {
    ShortDescriptionRole = MetaPlaylistModel::CustomRoleOffset,
    LongDescriptionRole
};

/* TODO: these should be replaced with custom roles for PlaylistColumn so all data of a playlist can
   be fetched at once with itemData() */
enum
{
    SubtitleColumn = MetaPlaylistModel::CustomColumOffset,
    AuthorColumn,
    KeywordsColumn,
    FilesizeColumn, // episode only
    ImageColumn,    // channel only (for now)
    DateColumn,
    IsEpisodeColumn,
    OnDiskColumn,
    ColumnCount
};

/**
    @author Bart Cerneels
*/
class PodcastModel : public QAbstractItemModel, public MetaPlaylistModel,
                     public Playlists::PlaylistObserver
{
    Q_OBJECT
    public:
        static PodcastModel *instance();
        static void destroy();

        virtual QVariant data(const QModelIndex &index, int role) const;
        virtual bool setData( const QModelIndex &index, const QVariant &value,
                              int role = Qt::EditRole );
        virtual Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;
        virtual QModelIndex index(int row, int column,
                        const QModelIndex &parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex &index) const;
        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

        virtual QStringList mimeTypes() const;
        virtual QMimeData* mimeData( const QModelIndexList &indexes ) const;
        virtual bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row,
                                   int column, const QModelIndex & parent );

        //MetaPlaylistModel methods
        virtual QList<QAction *> actionsFor( const QModelIndexList &indexes );
        virtual void loadItems( QModelIndexList list, Playlist::AddOptions insertMode );

        /* Meta::PlaylistObserver methods */
        virtual void trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track,
                                 int position );
        virtual void trackRemoved( Playlists::PlaylistPtr playlist, int position );

        //Own methods
        void downloadItems(  QModelIndexList list );
        void deleteItems(  QModelIndexList list );
        void refreshItems( QModelIndexList list );
        void removeSubscription( QModelIndexList list );

        /** @returns all channels currently selected
        **/
        Podcasts::PodcastChannelList selectedChannels() { return m_selectedChannels; }

        /** @returns all episodes currently selected, this includes children of a selected
        * channel
        **/
        Podcasts::PodcastEpisodeList selectedEpisodes() { return m_selectedEpisodes; }

        void importOpml( const KUrl &url );

    signals:
        void renameIndex( const QModelIndex &index ); // TODO: this signal is not being used atm

    public slots:
        void slotUpdate();
        void addPodcast();
        void refreshPodcasts();
        void setPodcastsInterval();
        void emitLayoutChanged();

    private slots:
        void slotAppend();
        void slotLoad();
        void slotSetNew( bool newState );
        void slotOpmlOutlineParsed( OpmlOutline* );
        void slotOpmlParsingDone();

    private:
        static PodcastModel* s_instance;
        PodcastModel();
        ~PodcastModel();
        static int podcastItemType( const QModelIndex &index );
        static Podcasts::PodcastChannelPtr channelForIndex( const QModelIndex &index );
        static Podcasts::PodcastEpisodePtr episodeForIndex( const QModelIndex &index );

        Q_DISABLE_COPY( PodcastModel )

        Playlists::PlaylistProvider *getProviderByName( const QString &name );
        Podcasts::PodcastChannelList selectedChannels( const QModelIndexList &indices );
        Podcasts::PodcastEpisodeList selectedEpisodes( const QModelIndexList &indices );
        QList<QAction *> createCommonActions( QModelIndexList indices );
        QList< QAction * > createEpisodeActions( Podcasts::PodcastEpisodeList epsiodes );
        QAction * m_appendAction;
        QAction * m_loadAction;
        QAction *m_setNewAction;
        Podcasts::PodcastEpisodeList m_selectedEpisodes;
        Podcasts::PodcastChannelList m_selectedChannels;

        /** A convenience function to convert a PodcastEpisodeList into a TrackList.
        **/
        static Meta::TrackList
        podcastEpisodesToTracks(
            Podcasts::PodcastEpisodeList episodes );

        /** Get the provider associated with a PodcastMetaCommon object */
        Podcasts::PodcastProvider *providerForPmc( Podcasts::PodcastMetaCommon *pmc ) const;

        void downloadEpisode( Podcasts::PodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode );
        void refreshPodcast( Podcasts::PodcastChannelPtr channel );
        Podcasts::PodcastChannelList m_channels;
        void removeSubscription( Podcasts::PodcastChannelPtr channel );
        void configureChannel( Podcasts::PodcastChannelPtr channel );

        bool isOnDisk( Podcasts::PodcastMetaCommon *pmc ) const;
        QVariant icon( Podcasts::PodcastMetaCommon *pmc ) const;
};

}

namespace The {
    AMAROK_EXPORT PlaylistBrowserNS::PodcastModel* podcastModel();
}

#endif
