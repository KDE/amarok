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

#ifndef UMSPODCASTCAPABILITY_H
#define UMSPODCASTCAPABILITY_H

#include "PodcastCapability.h"

class UmsPodcastCapability : public Handler::PodcastCapability
{
public:
    UmsPodcastCapability();
    ~UmsPodcastCapability();

    virtual void prepareToParsePodcasts();
    virtual bool isEndOfParsePodcastsList();
    virtual void prepareToParseNextPodcast();
    virtual void nextPodcastToParse();
    virtual bool shouldNotParseNextPodcast();

    virtual void prepareToParsePodcastEpisode();
    virtual bool isEndOfParsePodcast();
    virtual void prepareToParseNextPodcastEpisode();
    virtual void nextPodcastEpisodeToParse();
    virtual Handler::MediaDevicePodcastEpisodePtr libGetEpisodePtrForEpisodeStruct();
    virtual QString libGetPodcastName();
    
    virtual void addPodcast( const Meta::PodcastChannelPtr &channel ) { Q_UNUSED( channel ); }
    virtual void removePodcast( const Handler::MediaDevicePodcastChannelPtr &channel ) { Q_UNUSED( channel ); }
    virtual void removePodcastEpisode( const Handler::MediaDevicePodcastEpisodePtr &episode ) { Q_UNUSED( episode ); }

};

#endif // UMSPODCASTCAPABILITY_H
