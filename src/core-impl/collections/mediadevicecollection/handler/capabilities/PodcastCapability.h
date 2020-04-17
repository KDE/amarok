/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef PODCASTCAPABILITY_H
#define PODCASTCAPABILITY_H

#include "core-impl/collections/mediadevicecollection/handler/MediaDeviceHandlerCapability.h"
#include "core-impl/collections/mediadevicecollection/podcast/MediaDevicePodcastMeta.h"

namespace Handler
{
    class MEDIADEVICECOLLECTION_EXPORT PodcastCapability : public Handler::Capability
    {
        public:
            ~PodcastCapability() override;

            /**
             * This method initializes iteration over some list of Podcast structs
             * e.g. with libgpod, this initializes a GList to the beginning of
             * the list of Podcasts
             */
            virtual void prepareToParsePodcasts() = 0;

            /**
             * This method runs a test to see if we have reached the end of
             * the list of Podcasts to be parsed on the device, e.g. in libgpod
             * this tests if cur != NULL, i.e. if(cur)
             */
            virtual bool isEndOfParsePodcastsList() = 0;

            /**
             * This method moves the iterator to the next Podcast on the list of
             * Podcast structs, e.g. with libgpod, cur = cur->next where cur
             * is a GList*
             */
            virtual void prepareToParseNextPodcast() = 0;

            /**
             * This method attempts to access the special struct of the
             * next Podcast, so that information can then be parsed from it.
             * For libgpod, this is m_currPodcast = ( Itdb_Podcast * ) cur->data
             */
            virtual void nextPodcastToParse() = 0;

            /**
             * This method checks if the Podcast should be parsed, or skipped.
             * Certain Podcasts, like the master Podcast on the iPod, do not
             * need to be or should not be parsed.
             * @return true if should not parse, false otherwise.
             */
            virtual bool shouldNotParseNextPodcast() = 0;

            /**
             * This method initializes iteration over some list of track structs
             * that correspond to a Podcast struct
             * e.g. with libgpod, this initializes a GList to the beginning of
             * the list of tracks
             */
            virtual void prepareToParsePodcastEpisode() = 0;

            /**
             * This method runs a test to see if we have reached the end of
             * the list of episode in the Podcast to be parsed on the device, e.g. in libgpod
             * this tests if cur != NULL, i.e. if(cur)
             */
            virtual bool isEndOfParsePodcast() = 0;

            /**
             * This method moves the iterator to the next track on the Podcast of
             * track structs, e.g. with libgpod, cur = cur->next where cur
             * is a GList*
             */
            virtual void prepareToParseNextPodcastEpisode() = 0;

            /**
             * This method attempts to access the special struct of the
             * next track on the Podcast, so that information can then be parsed from it.
             * For libgpod, this is m_currtrack = (Itdb_Track*) cur->data
             */
            virtual void nextPodcastEpisodeToParse() = 0;

            /**
             * Returns a MediaDeviceTrackPtr that is associated with the currently parsed track struct.
             * @return A MediaDeviceTrackPtr to currently parsed track struct
             */
            virtual MediaDevicePodcastEpisodePtr libGetEpisodePtrForEpisodeStruct() = 0;

            /**
             * Returns a string containing the Podcast name of the currently parsed Podcast struct, if available.
             * @return A string with the name of the currently parsed Podcast
             */
            virtual QString libGetPodcastName() = 0;

            /**
             * Adds a podcast
             */
            virtual void addPodcast( const Podcasts::PodcastChannelPtr &channel ) = 0;

            /**
             * Deletes a particular Podcast from the device
             * @param channel the channel to remove
             */
            virtual void removePodcast( const MediaDevicePodcastChannelPtr &channel ) = 0;

            /**
             * Deletes a particular Podcast Episode from the device
             * @param episode the episode to remove
             */
            virtual void removePodcastEpisode( const MediaDevicePodcastEpisodePtr &episode ) = 0;

            /**
             * This method must create a two-way association of the current Podcasts::Podcast
             * to the special struct provided by the library to read/write information.
             * For example, for libgpod one would associate Itdb_Podcast*.  It makes
             * the most sense to use a QHash since it is fastest lookup and order
             * does not matter.
             * @param channel The channel to two-way associate with a library list struct
             */
            virtual void setAssociatePodcast( const MediaDevicePodcastChannelPtr &channel ) { Q_UNUSED( channel ) }

            static Type capabilityInterfaceType() { return Handler::Capability::Podcast; }
    };
}

#endif
