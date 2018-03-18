/****************************************************************************************
 * Copyright (c) 2011 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2011 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2011 Felix Winter <ixos01@gmail.com>                                   *
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
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

#ifndef GPODDERPODCASTMETA_H
#define GPODDERPODCASTMETA_H

#include "core/playlists/PlaylistProvider.h"
#include "core/podcasts/PodcastMeta.h"
#include <mygpo-qt5/ApiRequest.h>

namespace Podcasts {

class GpodderPodcastChannel;
class GpodderProvider;

typedef AmarokSharedPointer<GpodderPodcastChannel> GpodderPodcastChannelPtr;
typedef QList<GpodderPodcastChannelPtr> GpodderPodcastChannelList;

class GpodderPodcastChannel : public PodcastChannel
{
    public:
        explicit GpodderPodcastChannel( GpodderProvider *provider );

        //Copy a PodcastChannel
        GpodderPodcastChannel( GpodderProvider *provider, PodcastChannelPtr channel );

        //Create a channel from a mygpo podcast channel
        GpodderPodcastChannel( GpodderProvider *provider, mygpo::PodcastPtr channel );

        //PodcastChannel Methods
        virtual Playlists::PlaylistProvider *provider() const;

        //Playlist virtual methods
        virtual QUrl uidUrl() const;

    private:
        GpodderProvider *m_provider;
};

}

Q_DECLARE_METATYPE( Podcasts::GpodderPodcastChannelPtr )
Q_DECLARE_METATYPE( Podcasts::GpodderPodcastChannelList )

#endif
