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

#include "PodcastMeta.h"

#include <KJob>

class PodcastImageFetcher : public QObject
{
Q_OBJECT
public:
    PodcastImageFetcher();

    void addChannel( Meta::PodcastChannelPtr channel );
    void addEpisode( Meta::PodcastEpisodePtr episode );

    void run();

    static KUrl cachedImagePath( Meta::PodcastChannelPtr channel );

signals:
    void imageReady( Meta::PodcastChannelPtr channel, QPixmap image );
    void imageReady( Meta::PodcastEpisodePtr episode, QPixmap image );
    void done( PodcastImageFetcher * );

private slots:
    void slotDownloadFinished( KJob *job );

private:
    bool hasCachedImage( Meta::PodcastChannelPtr channel );

    Meta::PodcastChannelList m_channels;
    Meta::PodcastEpisodeList m_episodes;
    QMap<KJob *, Meta::PodcastChannelPtr> m_jobChannelMap;
    QMap<KJob *, Meta::PodcastEpisodePtr> m_jobEpisodeMap;
};

#endif // PODCASTIMAGEFETCHER_H
