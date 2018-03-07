/****************************************************************************************
 * Copyright 2010 Bart Cerneels <bart.cerneels@kde.org>                                 *
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


#ifndef MEDIADEVICEPODCASTMETA_H
#define MEDIADEVICEPODCASTMETA_H

#endif // MEDIADEVICEPODCASTMETA_H

#include "core/podcasts/PodcastMeta.h"

namespace Handler
{
    class MediaDevicePodcastChannel;
    typedef AmarokSharedPointer<MediaDevicePodcastChannel> MediaDevicePodcastChannelPtr;
    typedef QList<MediaDevicePodcastChannelPtr> MediaDevicePodcastChannelList;

    class MediaDevicePodcastEpisode;
    typedef AmarokSharedPointer<MediaDevicePodcastEpisode> MediaDevicePodcastEpisodePtr;
    typedef QList<MediaDevicePodcastEpisodePtr> MediaDevicePodcastEpisodeList;

    class MediaDevicePodcastChannel : public Podcasts::PodcastChannel
    {

    };

    class MediaDevicePodcastEpisode : public Podcasts::PodcastEpisode
    {

    };
}

Q_DECLARE_METATYPE( Handler::MediaDevicePodcastChannelPtr )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastChannelList )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastEpisodePtr )
Q_DECLARE_METATYPE( Handler::MediaDevicePodcastEpisodeList )
