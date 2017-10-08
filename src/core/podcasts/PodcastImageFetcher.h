/****************************************************************************************
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef PODCASTIMAGEFETCHER_H
#define PODCASTIMAGEFETCHER_H

#include "core/podcasts/PodcastMeta.h"

#include <KJob>

class AMAROK_CORE_EXPORT PodcastImageFetcher : public QObject
{
Q_OBJECT
public:
    PodcastImageFetcher();

    void addChannel( Podcasts::PodcastChannelPtr channel );
    void addEpisode( Podcasts::PodcastEpisodePtr episode );

    void run();

    static QUrl cachedImagePath( Podcasts::PodcastChannelPtr channel );
    static QUrl cachedImagePath( Podcasts::PodcastChannel *channel );

Q_SIGNALS:
    void channelImageReady( Podcasts::PodcastChannelPtr channel, QImage image );
    void episodeImageReady( Podcasts::PodcastEpisodePtr episode, QImage image );
    void done( PodcastImageFetcher * );

private Q_SLOTS:
    void slotDownloadFinished( KJob *job );

private:
    static bool hasCachedImage( Podcasts::PodcastChannelPtr channel );

    Podcasts::PodcastChannelList m_channels;
    Podcasts::PodcastEpisodeList m_episodes;
    QMap<KJob *, Podcasts::PodcastChannelPtr> m_jobChannelMap;
    QMap<KJob *, Podcasts::PodcastEpisodePtr> m_jobEpisodeMap;
};

#endif // PODCASTIMAGEFETCHER_H
