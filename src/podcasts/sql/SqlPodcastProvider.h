/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef SQLPODCASTPROVIDER_H
#define SQLPODCASTPROVIDER_H

#include "PodcastProvider.h"
#include "SqlPodcastMeta.h"

#include <kio/jobclasses.h>
#include <klocale.h>

class KUrl;
class PodcastReader;
class SqlStorage;

/**
	@author Bart Cerneels <bart.cerneels@kde.org>
*/
class SqlPodcastProvider : public PodcastProvider
{
    Q_OBJECT
    public:
        SqlPodcastProvider();
        ~SqlPodcastProvider();

        bool possiblyContainsTrack( const KUrl &url ) const;
        Meta::TrackPtr trackForUrl( const KUrl &url );

        QString prettyName() const { return i18n("Local Podcasts"); };
        int category() const { return (int)PlaylistManager::PodcastChannel; };

        Meta::PlaylistList playlists();

        //PodcastProvider methods
        void addPodcast( const KUrl &url );

        Meta::PodcastChannelPtr addChannel( Meta::PodcastChannelPtr channel );
        Meta::PodcastEpisodePtr addEpisode( Meta::PodcastEpisodePtr episode );

        Meta::PodcastChannelList channels();

        //SqlPodcastProvider specific methods
        Meta::SqlPodcastChannelPtr podcastChannelForId( int podcastChannelDbId );

    public slots:
        void updateAll();
        void update( Meta::PodcastChannelPtr channel );
        void downloadEpisode( Meta::PodcastEpisodePtr episode );
        void slotUpdated();

        void slotReadResult( PodcastReader *podcastReader, bool result );
        void update( Meta::SqlPodcastChannelPtr channel );
        void downloadEpisode( Meta::SqlPodcastEpisodePtr episode );

    private slots:
        void downloadResult( KJob * );
        void redirected( KIO::Job *, const KUrl& );

    signals:
            void updated();

    private:
        /** creates all the necessary tables, indexes etc. for the database */
        void createTables() const;
        void loadPodcasts();
        void updateDatabase( int fromVersion, int toVersion );

        Meta::SqlPodcastChannelList m_channels;

        QHash<KJob *, Meta::SqlPodcastEpisode *> m_jobMap;
        QHash<KJob *, QString> m_fileNameMap;

};

#endif
