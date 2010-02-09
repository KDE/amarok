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

#include "meta/PlaylistFile.h"
#include "podcasts/PodcastMeta.h"

#include "meta/file/File.h"

class UmsPodcastEpisode;
class UmsPodcastChannel;
class UmsPodcastProvider;

class KUrl;

typedef KSharedPtr<UmsPodcastEpisode> UmsPodcastEpisodePtr;
typedef KSharedPtr<UmsPodcastChannel> UmsPodcastChannelPtr;

typedef QList<UmsPodcastEpisodePtr> UmsPodcastEpisodeList;
typedef QList<UmsPodcastChannelPtr> UmsPodcastChannelList;

class UmsPodcastEpisode : public Meta::PodcastEpisode
{
    friend class UmsPodcastProvider;

    public:
        static UmsPodcastEpisodePtr fromPodcastEpisodePtr( Meta::PodcastEpisodePtr episode );
        static Meta::PodcastEpisodePtr toPodcastEpisodePtr( UmsPodcastEpisodePtr episode );
        static Meta::PodcastEpisodeList toPodcastEpisodeList( UmsPodcastEpisodeList episodes );

        UmsPodcastEpisode( UmsPodcastChannelPtr channel );
        ~UmsPodcastEpisode();

        void setLocalFile( MetaFile::TrackPtr localFile );

        //PodcastEpisode methods
        virtual QString title() const;
        virtual void setLocalUrl( const KUrl &localUrl );

        //Track Methods
        virtual QString name() const { return title(); }
        virtual KUrl playableUrl() const;
        virtual QString prettyName() const { return name(); }
        virtual void setTitle( const QString &title );
        virtual bool isEditable() const;

        virtual Meta::AlbumPtr album() const;
        virtual Meta::ArtistPtr artist() const;
        virtual Meta::ComposerPtr composer() const;
        virtual Meta::GenrePtr genre() const;
        virtual Meta::YearPtr year() const;

    private:
        MetaFile::TrackPtr m_localFile;
        UmsPodcastChannelPtr m_umsChannel;
};

class UmsPodcastChannel : public Meta::PodcastChannel
{
    friend class UmsPodcastProvider;
    public:
        static UmsPodcastChannelPtr fromPodcastChannelPtr(
                Meta::PodcastChannelPtr channel );
        static Meta::PodcastChannelPtr toPodcastChannelPtr( UmsPodcastChannelPtr channel );
        static Meta::PodcastChannelList toPodcastChannelList(
                UmsPodcastChannelList umsChannels );

        UmsPodcastChannel( UmsPodcastProvider *provider );
        UmsPodcastChannel( Meta::PodcastChannelPtr channel, UmsPodcastProvider *provider );
        ~UmsPodcastChannel();

        UmsPodcastEpisodeList umsEpisodes() { return m_umsEpisodes; }
        void addUmsEpisode( UmsPodcastEpisodePtr episode ) { m_umsEpisodes << episode; }

        void setPlaylistFileSource( const KUrl &playlistFilePath );
        KUrl playlistFilePath() const { return m_playlistFilePath; }

        virtual Meta::PodcastEpisodeList episodes()
                { return UmsPodcastEpisode::toPodcastEpisodeList( m_umsEpisodes ); }
        virtual PlaylistProvider *provider() const;

    private:
        UmsPodcastProvider *m_provider;
        KUrl m_playlistFilePath;
        Meta::PlaylistFilePtr m_playlistFile; //used to keep track of episodes.

        UmsPodcastEpisodeList m_umsEpisodes;
};

Q_DECLARE_METATYPE( UmsPodcastEpisodePtr )
Q_DECLARE_METATYPE( UmsPodcastEpisodeList )
Q_DECLARE_METATYPE( UmsPodcastChannelPtr )
Q_DECLARE_METATYPE( UmsPodcastChannelList )

#endif // UMSPODCASTMETA_H
