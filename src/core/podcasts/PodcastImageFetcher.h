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

class AMAROK_EXPORT PodcastImageFetcher : public QObject
{
Q_OBJECT
public:
    PodcastImageFetcher();

    void addChannel( Podcasts::PodcastChannelPtr channel );
    void addEpisode( Podcasts::PodcastEpisodePtr episode );

    void run();

    static KUrl cachedImagePath( Podcasts::PodcastChannelPtr channel );
    static KUrl cachedImagePath( Podcasts::PodcastChannel *channel );

signals:
    void imageReady( Podcasts::PodcastChannelPtr channel, QPixmap image );
    void imageReady( Podcasts::PodcastEpisodePtr episode, QPixmap image );
    void done( PodcastImageFetcher * );

private slots:
    void slotDownloadFinished( KJob *job );

private:
    static bool hasCachedImage( Podcasts::PodcastChannelPtr channel );

    Podcasts::PodcastChannelList m_channels;
    Podcasts::PodcastEpisodeList m_episodes;
    QMap<KJob *, Podcasts::PodcastChannelPtr> m_jobChannelMap;
    QMap<KJob *, Podcasts::PodcastEpisodePtr> m_jobEpisodeMap;
};

#endif // PODCASTIMAGEFETCHER_H
