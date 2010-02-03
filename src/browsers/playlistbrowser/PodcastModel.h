/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PodcastMeta.h"

#include "MetaPlaylistModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistController.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QVariant>

class OpmlOutline;
class QAction;

namespace PlaylistBrowserNS {

enum {
    ShortDescriptionRole = Qt::UserRole + 1,
    LongDescriptionRole
};

enum
{
    TitleColumn,
    SubtitleColumn,
    AuthorColumn,
    KeywordsColumn,
    FilesizeColumn, // episode only
    ImageColumn,    // channel only (for now)
    DateColumn,
    IsEpisodeColumn,
    ProviderColumn,
    OnDiskColumn,
    ColumnCount
};

/**
    @author Bart Cerneels
*/
class PodcastModel : public QAbstractItemModel, public MetaPlaylistModel
{
    Q_OBJECT
    public:
        static PodcastModel *instance();
        static void destroy();

        virtual QVariant data(const QModelIndex &index, int role) const;
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

        //Own methods
        void downloadItems(  QModelIndexList list );
        void deleteItems(  QModelIndexList list );
        void refreshItems( QModelIndexList list );
        void removeSubscription( QModelIndexList list );
        void configureChannels( QModelIndexList list );

        /** @returns all channels currently selected
        **/
        Meta::PodcastChannelList selectedChannels() { return m_selectedChannels; }

        /** @returns all episodes currently selected, this includes children of a selected
        * channel
        **/
        Meta::PodcastEpisodeList selectedEpisodes() { return m_selectedEpisodes; }

        void importOpml( const KUrl &url );

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

        Q_DISABLE_COPY( PodcastModel )

        Meta::PodcastChannelList selectedChannels( const QModelIndexList &indices );
        Meta::PodcastEpisodeList selectedEpisodes( const QModelIndexList &indices );
        QList<QAction *> createCommonActions( QModelIndexList indices );
        QList< QAction * > createEpisodeActions( Meta::PodcastEpisodeList epsiodes );
        QAction * m_appendAction;
        QAction * m_loadAction;
        QAction *m_setNewAction;
        Meta::PodcastEpisodeList m_selectedEpisodes;
        Meta::PodcastChannelList m_selectedChannels;

        /** A convenience function to convert a PodcastEpisodeList into a TrackList.
        **/
        static Meta::TrackList
        podcastEpisodesToTracks(
            Meta::PodcastEpisodeList episodes );

        void downloadEpisode( Meta::PodcastEpisodePtr episode );
        void deleteDownloadedEpisode( Meta::PodcastEpisodePtr episode );
        void refreshPodcast( Meta::PodcastChannelPtr channel );
        Meta::PodcastChannelList m_channels;
        void removeSubscription( Meta::PodcastChannelPtr channel );
        void configureChannel( Meta::PodcastChannelPtr channel );

        bool isOnDisk( Meta::PodcastMetaCommon *pmc ) const;
        QVariant icon( Meta::PodcastMetaCommon *pmc ) const;
};

}

namespace The {
    AMAROK_EXPORT PlaylistBrowserNS::PodcastModel* podcastModel();
}

#endif
