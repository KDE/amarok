/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef UMSPODCASTMETA_H
#define UMSPODCASTMETA_H

#include "core-impl/playlists/types/file/PlaylistFile.h"
#include "core/podcasts/PodcastMeta.h"

#include "core-impl/meta/file/File.h"

class QUrl;

namespace Podcasts {

class UmsPodcastEpisode;
class UmsPodcastChannel;
class UmsPodcastProvider;


typedef AmarokSharedPointer<UmsPodcastEpisode> UmsPodcastEpisodePtr;
typedef AmarokSharedPointer<UmsPodcastChannel> UmsPodcastChannelPtr;

typedef QList<UmsPodcastEpisodePtr> UmsPodcastEpisodeList;
typedef QList<UmsPodcastChannelPtr> UmsPodcastChannelList;

class UmsPodcastEpisode : public Podcasts::PodcastEpisode
{
    friend class UmsPodcastProvider;

    public:
        static UmsPodcastEpisodePtr fromPodcastEpisodePtr( Podcasts::PodcastEpisodePtr episode );
        static UmsPodcastEpisodePtr fromTrackPtr( Meta::TrackPtr track );
        static Podcasts::PodcastEpisodePtr toPodcastEpisodePtr( UmsPodcastEpisodePtr episode );
        static Podcasts::PodcastEpisodeList toPodcastEpisodeList( UmsPodcastEpisodeList episodes );

        explicit UmsPodcastEpisode( UmsPodcastChannelPtr channel );
        ~UmsPodcastEpisode();

        void setLocalFile( MetaFile::TrackPtr localFile );

        //PodcastEpisode methods
        QString title() const override;
        void setLocalUrl( const QUrl &localUrl ) override;

        //Track Methods
        QString name() const override { return title(); }
        QUrl playableUrl() const override;
        QString notPlayableReason() const override;
        QString prettyName() const override { return name(); }
        void setTitle( const QString &title ) override;
        QDateTime createDate() const override;

        Meta::AlbumPtr album() const override;
        Meta::ArtistPtr artist() const override;
        Meta::ComposerPtr composer() const override;
        Meta::GenrePtr genre() const override;
        Meta::YearPtr year() const override;

    private:
        MetaFile::TrackPtr m_localFile;
        UmsPodcastChannelPtr m_umsChannel;
};

class UmsPodcastChannel : public Podcasts::PodcastChannel
{
    friend class UmsPodcastProvider;
    public:
        static UmsPodcastChannelPtr fromPodcastChannelPtr(
                Podcasts::PodcastChannelPtr channel );
        static Podcasts::PodcastChannelPtr toPodcastChannelPtr( UmsPodcastChannelPtr channel );
        static Podcasts::PodcastChannelList toPodcastChannelList(
                UmsPodcastChannelList umsChannels );

        explicit UmsPodcastChannel( UmsPodcastProvider *provider );
        UmsPodcastChannel( Podcasts::PodcastChannelPtr channel, UmsPodcastProvider *provider );
        ~UmsPodcastChannel();

        Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode ) override;

        UmsPodcastEpisodeList umsEpisodes() { return m_umsEpisodes; }
        void addUmsEpisode( UmsPodcastEpisodePtr episode );

        void setPlaylistFileSource( const QUrl &playlistFilePath );
        QUrl playlistFilePath() const { return m_playlistFilePath; }

        Podcasts::PodcastEpisodeList episodes() const override
                { return UmsPodcastEpisode::toPodcastEpisodeList( m_umsEpisodes ); }
        Playlists::PlaylistProvider *provider() const override;

    protected:
        void removeEpisode( UmsPodcastEpisodePtr episode );

    private:
        UmsPodcastProvider *m_provider;
        QUrl m_playlistFilePath;
        Playlists::PlaylistFilePtr m_playlistFile; //used to keep track of episodes.

        UmsPodcastEpisodeList m_umsEpisodes;
};

} //namespace Podcasts

Q_DECLARE_METATYPE( Podcasts::UmsPodcastEpisodePtr )
Q_DECLARE_METATYPE( Podcasts::UmsPodcastEpisodeList )
Q_DECLARE_METATYPE( Podcasts::UmsPodcastChannelPtr )
Q_DECLARE_METATYPE( Podcasts::UmsPodcastChannelList )

#endif // UMSPODCASTMETA_H
