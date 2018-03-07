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

#define DEBUG_PREFIX "GpodderPodcastMeta"

#include "GpodderPodcastMeta.h"

#include "GpodderProvider.h"

using namespace Podcasts;

Podcasts::GpodderPodcastChannel::GpodderPodcastChannel( GpodderProvider *provider )
    : PodcastChannel()
    , m_provider( provider )
{
}

Podcasts::GpodderPodcastChannel::GpodderPodcastChannel( GpodderProvider *provider,
                                                        PodcastChannelPtr channel )
    : PodcastChannel( channel )
    , m_provider( provider )
{
}

Podcasts::GpodderPodcastChannel::GpodderPodcastChannel( GpodderProvider *provider,
                                                        mygpo::PodcastPtr channel )
    : PodcastChannel()
    , m_provider( provider )
{
    setUrl( channel->url() );
    setWebLink( channel->website() );
    setImageUrl( channel->logoUrl() );
    setDescription( channel->description() );
    setTitle( channel->title() );
}

Playlists::PlaylistProvider *
Podcasts::GpodderPodcastChannel::provider() const
{
    return dynamic_cast<Playlists::PlaylistProvider *>( m_provider );
}


QUrl
Podcasts::GpodderPodcastChannel::uidUrl() const
{
    QUrl u = url();
    u.setScheme( QStringLiteral( "amarok-gpodder" ) );
    return u;
}
